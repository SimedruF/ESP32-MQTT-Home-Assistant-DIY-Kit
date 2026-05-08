#!/bin/bash
#
# Advanced PDF Generation Script with Configuration Support
# KY-026 Flame Sensor Documentation Generator
#
# Usage:
#   ./generate_pdf_advanced.sh                    # Use config file
#   ./generate_pdf_advanced.sh --all              # Generate all enabled PDFs
#   ./generate_pdf_advanced.sh --combine          # Generate combined PDF
#   ./generate_pdf_advanced.sh file1.html [file2.html ...] -o output.pdf
#

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Default values
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_FILE="$SCRIPT_DIR/pdf_config.json"
TEMP_DIR="/tmp/pdf_gen_$$"
COMBINE_MODE=false
OUTPUT_FILE=""

# Function to print colored messages
print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# Check dependencies
check_dependencies() {
    local missing_deps=()
    
    if ! command -v chromium &> /dev/null; then
        missing_deps+=("chromium-browser")
    fi
    
    if ! command -v pdfunite &> /dev/null; then
        missing_deps+=("poppler-utils")
    fi
    
    if ! command -v jq &> /dev/null; then
        missing_deps+=("jq")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        echo "Install with: sudo apt install ${missing_deps[*]}"
        return 1
    fi
    
    return 0
}

# Generate single PDF from HTML
generate_pdf() {
    local html_file="$1"
    local pdf_file="$2"
    
    # Convert to absolute paths
    if [[ ! "$html_file" = /* ]]; then
        html_file="$SCRIPT_DIR/$html_file"
    fi
    if [[ ! "$pdf_file" = /* ]]; then
        pdf_file="$SCRIPT_DIR/$pdf_file"
    fi
    
    if [ ! -f "$html_file" ]; then
        print_error "HTML file not found: $html_file"
        return 1
    fi
    
    local html_dir=$(dirname "$html_file")
    local html_name=$(basename "$html_file")
    local pdf_name=$(basename "$pdf_file")
    
    cd "$html_dir" || return 1
    
    chromium --headless --disable-gpu --print-to-pdf="$pdf_name" \
             --print-to-pdf-no-header "$html_name" 2>&1 | grep -q "bytes written"
    
    local result=$?
    local temp_pdf="$html_dir/$pdf_name"
    
    if [ $result -eq 0 ] && [ -f "$temp_pdf" ]; then
        # Move to correct location if needed
        if [ "$temp_pdf" != "$pdf_file" ]; then
            mkdir -p "$(dirname "$pdf_file")"
            mv "$temp_pdf" "$pdf_file"
        fi
        cd "$SCRIPT_DIR"
        return 0
    fi
    
    cd "$SCRIPT_DIR"
    return 1
}

# Combine multiple HTML files into single PDF
combine_pdfs() {
    local output="$1"
    shift
    local html_files=("$@")
    
    print_info "Combining ${#html_files[@]} HTML files into single PDF..."
    
    mkdir -p "$TEMP_DIR"
    
    local temp_pdfs=()
    local index=1
    
    for html_file in "${html_files[@]}"; do
        print_info "Processing [$index/${#html_files[@]}]: $(basename "$html_file")"
        
        local temp_pdf="$TEMP_DIR/temp_$(printf "%03d" $index).pdf"
        
        if generate_pdf "$html_file" "$temp_pdf"; then
            temp_pdfs+=("$temp_pdf")
            print_success "Generated temporary PDF: $(basename "$temp_pdf")"
        else
            print_error "Failed to generate PDF from: $html_file"
            rm -rf "$TEMP_DIR"
            return 1
        fi
        
        ((index++))
    done
    
    # Combine all PDFs
    if [ ${#temp_pdfs[@]} -gt 0 ]; then
        print_info "Merging PDFs..."
        
        if pdfunite "${temp_pdfs[@]}" "$output" 2>/dev/null; then
            local size=$(du -h "$output" | cut -f1)
            print_success "Combined PDF created: $output ($size)"
            rm -rf "$TEMP_DIR"
            return 0
        else
            print_error "Failed to merge PDFs"
            rm -rf "$TEMP_DIR"
            return 1
        fi
    fi
    
    rm -rf "$TEMP_DIR"
    return 1
}

# Process configuration file
process_config() {
    if [ ! -f "$CONFIG_FILE" ]; then
        print_error "Configuration file not found: $CONFIG_FILE"
        return 1
    fi
    
    print_header "PDF Generation from Configuration"
    print_info "Config file: $CONFIG_FILE"
    echo ""
    
    # Get number of enabled documents
    local doc_count=$(jq '[.documents[] | select(.enabled == true)] | length' "$CONFIG_FILE")
    
    if [ "$doc_count" -eq 0 ]; then
        print_warning "No enabled documents in configuration"
        return 0
    fi
    
    print_info "Processing $doc_count enabled document(s)..."
    echo ""
    
    # Process each enabled document
    local index=0
    while [ $index -lt "$doc_count" ]; do
        local doc_name=$(jq -r ".documents[] | select(.enabled == true) | [$index] | .[0].name // empty" "$CONFIG_FILE" 2>/dev/null)
        
        # Get document at current index
        local doc=$(jq -c "[.documents[] | select(.enabled == true)][$index]" "$CONFIG_FILE")
        
        local name=$(echo "$doc" | jq -r '.name')
        local desc=$(echo "$doc" | jq -r '.description')
        local output_pdf=$(echo "$doc" | jq -r '.output_pdf')
        local html_count=$(echo "$doc" | jq -r '.html_files | length')
        
        print_info "[$((index+1))/$doc_count] $name - $desc"
        
        if [ "$html_count" -eq 1 ]; then
            # Single HTML file
            local html_file=$(echo "$doc" | jq -r '.html_files[0]')
            if generate_pdf "$html_file" "$output_pdf"; then
                local size=$(du -h "$output_pdf" | cut -f1)
                print_success "Generated: $output_pdf ($size)"
            else
                print_error "Failed to generate: $output_pdf"
            fi
        else
            # Multiple HTML files - combine
            local html_files=()
            for i in $(seq 0 $((html_count-1))); do
                html_files+=($(echo "$doc" | jq -r ".html_files[$i]"))
            done
            
            if combine_pdfs "$output_pdf" "${html_files[@]}"; then
                print_success "Combined PDF created successfully"
            else
                print_error "Failed to create combined PDF"
            fi
        fi
        
        echo ""
        ((index++))
    done
    
    return 0
}

# Command line processing
process_command_line() {
    local html_files=()
    local output=""
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -o|--output)
                output="$2"
                shift 2
                ;;
            -c|--config)
                CONFIG_FILE="$2"
                shift 2
                ;;
            --combine)
                COMBINE_MODE=true
                shift
                ;;
            --all)
                # Enable all documents in config
                process_config
                exit $?
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                if [[ "$1" == *.html ]]; then
                    html_files+=("$1")
                fi
                shift
                ;;
        esac
    done
    
    if [ ${#html_files[@]} -eq 0 ]; then
        # No files specified, use config
        process_config
        return $?
    fi
    
    if [ -z "$output" ]; then
        print_error "Output file not specified. Use -o output.pdf"
        return 1
    fi
    
    if [ ${#html_files[@]} -eq 1 ]; then
        print_header "Single PDF Generation"
        generate_pdf "${html_files[0]}" "$output"
    else
        print_header "Combined PDF Generation"
        combine_pdfs "$output" "${html_files[@]}"
    fi
}

show_help() {
    cat << EOF
Advanced PDF Generation Script

Usage:
    $0 [OPTIONS] [HTML_FILES...]

OPTIONS:
    --all               Generate all enabled PDFs from config file
    --combine           Enable combined mode for multiple HTML files
    -c, --config FILE   Use specific configuration file (default: pdf_config.json)
    -o, --output FILE   Output PDF filename
    -h, --help          Show this help message

EXAMPLES:
    # Generate from config file
    $0 --all

    # Generate single PDF
    $0 -o output.pdf input.html

    # Combine multiple HTML files
    $0 file1.html file2.html file3.html -o combined.pdf

    # Use custom config file
    $0 --all -c custom_config.json

CONFIGURATION FILE:
    The config file (pdf_config.json) should contain:
    {
      "documents": [
        {
          "name": "doc_name",
          "description": "Description",
          "html_files": ["file1.html", "file2.html"],
          "output_pdf": "output.pdf",
          "enabled": true
        }
      ]
    }

DEPENDENCIES:
    - chromium-browser
    - poppler-utils (pdfunite)
    - jq

EOF
}

# Main execution
main() {
    cd "$SCRIPT_DIR" || exit 1
    
    if ! check_dependencies; then
        exit 1
    fi
    
    if [ $# -eq 0 ]; then
        # No arguments - use config file
        process_config
    else
        # Process command line arguments
        process_command_line "$@"
    fi
    
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        print_header "PDF Generation Complete!"
    else
        print_header "PDF Generation Failed"
    fi
    
    exit $exit_code
}

main "$@"
