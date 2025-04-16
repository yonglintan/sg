#!/bin/bash
# Comprehensive test script for the SG language interpreter

# Color codes for better readability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_TESTS=0

echo -e "${YELLOW}=== SG Language Interpreter Test Suite ===${NC}\n"

# Function to run a single test case and check result
run_test_case() {
    local test_name=$1
    local input=$2
    local expected_output=$3
    local check_error=${4:-false}  # Whether to check for error messages instead of exact output
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Create temporary files
    local output_file=$(mktemp)
    
    # Run the test
    echo -e "$input" | ./sg > "$output_file" 2>&1
    
    # Read the content of the output file
    local output=$(<"$output_file")
    local test_passed=false
    
    # Check for errors if needed
    if [ "$check_error" = "true" ]; then
        if [[ "$output" == *"Error"* ]]; then
            test_passed=true
        fi
    else
        # For regular tests, check if the output contains the expected result
        if [[ "$output" == *"$expected_output"* ]]; then
            test_passed=true
        fi
    fi
    
    # Display test result
    if [ "$test_passed" = true ]; then
        echo -e "  ${BLUE}Test:${NC} $test_name: ${GREEN}PASS${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "  ${BLUE}Test:${NC} $test_name: ${RED}FAIL${NC}"
        echo -e "    ${BLUE}Expected:${NC} $expected_output"
        echo -e "    ${BLUE}Got:${NC}"
        cat "$output_file"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
    
    # Clean up
    rm "$output_file"
}

# Function to run a section of tests
run_test_section() {
    local section_name=$1
    
    echo -e "${YELLOW}=== Testing $section_name ===${NC}"
}

# ===== ARITHMETIC TESTS =====
run_test_section "Basic Arithmetic"
run_test_case "Addition" "1 + 2" "3"
run_test_case "Subtraction" "3 - 4" "-1"
run_test_case "Multiplication" "5 * 6" "30"
run_test_case "Division" "10 / 2" "5"
echo ""

# ===== GROUPING TESTS =====
run_test_section "Grouping"
run_test_case "Parentheses change precedence 1" "(1 + 2) * 3" "9"
run_test_case "Parentheses change precedence 2" "1 + (2 * 3)" "7"
echo ""

# ===== UNARY OPERATORS =====
run_test_section "Unary Operators"
run_test_case "Negation" "-10" "-10"
run_test_case "Logical not (true)" "!true" "false"
run_test_case "Logical not (false)" "!false" "true"
run_test_case "Double not (true)" "!!true" "true"
run_test_case "Double not (false)" "!!false" "false"
echo ""

# ===== COMPARISON OPERATORS =====
run_test_section "Comparison Operators"
run_test_case "Greater than (true)" "5 > 3" "true"
run_test_case "Less than (false)" "5 < 3" "false"
run_test_case "Greater than or equal (true)" "5 >= 5" "true"
run_test_case "Less than or equal (false)" "5 <= 4" "false"
echo ""

# ===== EQUALITY OPERATORS =====
run_test_section "Equality Operators"
run_test_case "Number equality (true)" "1 == 1" "true"
run_test_case "Number inequality (false)" "1 != 1" "false"
run_test_case "String equality (true)" '"hello" == "hello"' "true"
run_test_case "String equality (false)" '"hello" == "world"' "false"
run_test_case "Boolean equality" "true == true" "true"
run_test_case "Nil equality" "nil == nil" "true"
echo ""

# ===== STRING CONCATENATION =====
run_test_section "String Concatenation"
run_test_case "Basic concatenation" '"hello" + " world"' "hello world"
echo ""

# ===== MIXED EXPRESSIONS =====
run_test_section "Mixed Expressions"
run_test_case "Complex boolean expression" "1 + 2 * 3 > 5 == true" "true"
run_test_case "Left associative subtraction" "3 - 1 - 1" "1"
run_test_case "Operator precedence" "1 + 2 + 3 * 4 + 5" "20"
echo ""

# ===== RUNTIME ERRORS =====
run_test_section "Runtime Errors"
run_test_case "String subtraction" '"hello" - "world"' "Operands must be numbers", true
run_test_case "String multiplication" '5 * "test"' "Operands must be numbers", true
run_test_case "String negation" '-"string"' "Operand must be a number", true
run_test_case "String comparison" '"text" > 5' "Operands must be numbers", true
run_test_case "Division by zero" "10 / 0" "Division by zero", true
run_test_case "Complex expression error" '(5 + 10) * ("hello" - "world")' "Operands must be numbers", true
run_test_case "Type error in mixed expression" '(5 > 3) + 10' "Operands must be two numbers or two strings", true
echo ""

# ===== SCANNER ERRORS =====
run_test_section "Scanner Errors"
run_test_case "Invalid character" '@invalid_var' "Expect expression", true
run_test_case "Invalid symbols" '$$$' "Expect expression", true
run_test_case "Unterminated string" '"This string has no closing quote' "Expect expression", true
echo ""

# ===== SUMMARY =====
echo -e "${YELLOW}=== Test Summary ===${NC}"
echo -e "Total tests: ${TOTAL_TESTS}"
echo -e "${GREEN}Passed: ${PASS_COUNT}${NC}"
echo -e "${RED}Failed: ${FAIL_COUNT}${NC}"

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! 🎉${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed. 😢${NC}"
    exit 1
fi 