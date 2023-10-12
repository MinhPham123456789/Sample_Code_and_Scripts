#!/bin/bash

# This script searches the binaries inside the path "/bin" using 3 options. Option -s searches all binaries contains
# the provided input string. Option -b searches all binaries that meet the size requirement, the requirement should
# be in the format "operator,size". Option -l searches all binaries with a soft link. The script does not accept
# using multiple options at the same time.

# Usage example:
#  ./searchbin.sh -s vm  => this command searches all binaries containing the string "vm"
#  ./searchbin.sh -b lt,10 => this command searches all binaries with size less than 10 bytes 
#  Available operators: 
#   lt: less than
#   gt: greater than
#   le: less than or equal to
#   ge: greater than or equal to
#   eq: equal to
#   ne: not equal to
#  ./searchbin.sh -l => this command searches all binaries with a soft link

# Declare the output text color variables
BLUE=$'\033[0;34m'
GREEN=$'\033[0;32m'
RED=$'\033[0;31m'
NC=$'\033[0m'

# Declare option b argument array
declare -a option_b_args=()

# Bin directory path
search_path="/bin"

# Override OPTERR and set all options var to initial value
OPTERR="${RED}Invalid flag or missing argument error - exiting..${NC}"
input_option="begin"
option_s=0
option_b=0
option_l=0

# A function to check multiple options cases
multiple_options_validate()
{
    options_sum=$(echo "$option_s + $option_b + $option_l" | bc -l)
    # echo "$option_s + $option_b + $option_l"
    if [[ $options_sum -gt 1 ]]
    then
        echo -e "${RED}Try again with just a single option/argument(s).${NC}"
        exit 0
    fi
}

# A function to convert the item size to the suitable unit
interpret_size()
{
    let mb=1048576 # number of bytes in a megabyte
    let kb=1024 # number of bytes in a kilobyte
    if [[ $1 -ge $mb ]]
    then
        # When the size is greater than 1 megabyte divide the size twice by 1024 to convert it to MB unit
        echo "$(echo $1 | awk '{printf "%.2f", $1/1024/1024}')Mb"
    elif [[ $1 -ge $kb ]]
    then
        # When the size is greater than 1 kilobyte divide the size twice by 1024 to convert it to KB unit
        echo "$(echo $1 | awk '{printf "%.2f", $1/1024}')Kb"
    else
        echo "$1b"
    fi
}

# A function to print the result to the terminal
print_result()
{
    input_item=$1
    # Extract item's basename and assign to var
    item_name=$(basename $input_item)
    # Get the item's size and convert it to the biggest unit and print the name and size in terminal
    item_size=$(interpret_size $(du -b "$search_path/$input_item" | cut -f 1))
    printf "%-40s%-40s\n" $item_name $item_size
}

# A function to run option s operation
option_s_operation()
{
    option_s_arg=$1
    declare -a option_s_binaries_list=()
    # Iterate through the file in /bin/, only take binaries, get the binaries' name and only take the names matching the provided pattern
    for binary in $(ls -lhn $search_path/ | grep -ve "^d.*$" | awk 'NR>1{print $9}' | grep -iE ".*$option_s_arg.*")
    do
        option_s_binaries_list+=("$binary")  # Add matched name in the option_s_binaries_list array
    done
    # Check the option_s_binaries_list array length is greater than 0
    if [[ ${#option_s_binaries_list[@]} -gt 0 ]]
    then
        # When the option_s_binaries_list array is greater than 0, print the results in the array
        printf "${GREEN}%s${NC} %s\n" "${#option_s_binaries_list[@]}" "matches found..."
        printf "${BLUE}%-40s%-40s\n${NC}" "NAME" "SIZE"
        for s_item in "${option_s_binaries_list[@]}"
        do
            print_result $s_item
        done
    else
        # When the option_s_binaries_list array length is not greater than 0, print not found message
        echo -e "${BLUE}No matches found${NC}"
    fi
}

# A function to get and interpret option b's arguments
get_option_b_args()
{
    og_ifs=IFS
    IFS=','
    # Assign provided arguments to an array
    for arg in $1
    do
        option_b_args+=("$arg")
    done

    # Convert user input operator to lowercase
    option_b_args[0]=$(echo "${option_b_args[0]}" | awk '{print tolower($0)}')
    # Check the option_b_args array length and the arguments' content
    if [[ ! ${#option_b_args[@]} -eq 2 ]] || [[ ! ${option_b_args[0]} =~ ^(gt|lt|ge|le|eq|ne)$ ]]
    then
        # When the invalid arguments are provided, print warning message and exit
        echo -e "${RED}Invalid comparator/argument(s) passed. Exiting...${NC}"
        exit 0
    elif [[ ! ${option_b_args[1]} =~ ^[0-9]+$ ]]    # Check the second argument to be an integer
    then
        # When the invalid byte value is provided, print warning message and exit
        echo -e "${RED}Invalid byte value passed. Exiting...${NC}"
        exit 0
    fi
    IFS=og_ifs
}

# A function to run option b operation
option_b_operation()
{
    # Get the option b's arguments and declare a result array (option_b_binaries_list)
    operator=$1
    threshold_size=$2
    declare -a option_b_binaries_list

    og_ifs=IFS
    IFS=$'\n'
    # Check the operator and search the binaries that meet the requirements
    case $operator in
        gt) # Search binaries greater than a threshold size
        for b_item in $(ls -lhn $search_path/ | grep -ve "^d.*$" | awk 'NR>1{print $9}')
        do
            b_item_size=$(du -b "$search_path/$b_item" | cut -f 1)
            if [[ $b_item_size -gt $threshold_size ]]
            then
                option_b_binaries_list+=("$b_item")
            fi
        done
        ;;
        lt) # Search binaries less than a threshold size
        for b_item in $(ls -lhn $search_path/ | grep -ve "^d.*$" | awk 'NR>1{print $9}')
        do
            b_item_size=$(du -b "$search_path/$b_item" | cut -f 1)
            if [[ $b_item_size -lt $threshold_size ]]
            then
                option_b_binaries_list+=("$b_item")
            fi
        done
        ;;
        le) # Search binaries less than or equal a threshold size
        for b_item in $(ls -lhn $search_path/ | grep -ve "^d.*$" | awk 'NR>1{print $9}')
        do
            b_item_size=$(du -b "$search_path/$b_item" | cut -f 1)
            if [[ $b_item_size -le $threshold_size ]]
            then
                option_b_binaries_list+=("$b_item")
            fi
        done
        ;;
        ge) # Search binaries greater than or equal a threshold size
        for b_item in $(ls -lhn $search_path/ | grep -ve "^d.*$" | awk 'NR>1{print $9}')
        do
            b_item_size=$(du -b "$search_path/$b_item" | cut -f 1)
            if [[ $b_item_size -ge $threshold_size ]]
            then
                option_b_binaries_list+=("$b_item")
            fi
        done
        ;;
        eq) # Search binaries equal a threshold size
        for b_item in $(ls -lhn $search_path/ | grep -ve "^d.*$" | awk 'NR>1{print $9}')
        do
            b_item_size=$(du -b "$search_path/$b_item" | cut -f 1)
            if [[ $b_item_size -eq $threshold_size ]]
            then
                option_b_binaries_list+=("$b_item")
            fi
        done
        ;;
        ne) # Search binaries do not equal a threshold size
        for b_item in $(ls -lhn $search_path/ | grep -ve "^d.*$" | awk 'NR>1{print $9}')
        do
            b_item_size=$(du -b "$search_path/$b_item" | cut -f 1)
            if [[ $b_item_size -ne $threshold_size ]]
            then
                option_b_binaries_list+=("$b_item")
            fi
        done
        ;;
        *) # Invalid comparator/argument(s) provided
        echo -e "${RED}Invalid comparator/argument(s) passed. Exiting...${NC}"
        exit 0
        ;;
    esac
    # Check the option_b_binaries_list array length greater than 0
    if [[ ${#option_b_binaries_list[@]} -gt 0 ]]
    then
        # When the option_b_binaries_list array length greater than 0, print the result in the required layout
        printf "${GREEN}%s${NC} %s\n" "${#option_b_binaries_list[@]}" "matches found..."
        printf "${BLUE}%-40s%-40s\n${NC}" "NAME" "SIZE"
        for b_item in "${option_b_binaries_list[@]}"
        do
            print_result $b_item
        done
    else
        # When the option_b_binaries_list array length is not greater than 0, print not found message
        echo -e "${BLUE}No matches found${NC}"
    fi
    IFS=og_ifs
}

# A function to run l operation
option_l_operation()
{
    declare -a option_l_symbols_list
    declare -a option_l_destinations_list
    og_ifs=IFS
    IFS=$'\n'
    # Iterate the /bin/ and retrieves link symbols
    for l_item in $(ls -lhn $search_path/ | grep -e "^l.*$" | grep -w "^.*->.*$" | awk '{print $9}')
    do
        option_l_symbols_list+=("$l_item")  # Add the link symbol to option_l_symbols_list array
        # Add the symbol destination to option_l_symbols_list array
        option_l_destinations_list+=("$(ls -lhn $search_path/$l_item | awk '{print $NF}' | awk -F '/' '{print $NF}')")
    done
    # Check the option_l_symbols_list array length is greater than 0
    if [[ ${#option_l_symbols_list[@]} -gt 0 ]]
    then
        # When the option_l_symbols_list length is greater than 0, print the results in the required layout
        printf "${GREEN}%s${NC} %s\n" "${#option_l_symbols_list[@]}" "matches found..."
        printf "${BLUE}%-40s%-40s\n${NC}" "SYMBOLIC" "POINTS TO"
        for ((l_index=0; l_index<${#option_l_symbols_list[@]}; l_index++))
        do
            printf "%-40s%-40s\n" "${option_l_symbols_list[$l_index]}" "${option_l_destinations_list[$l_index]}"
        done
    else
        # When the option_l_symbols_list length is not greater than 0, print the not found message
        echo -e "${BLUE}No matches found${NC}"
    fi
    IFS=og_ifs
}

# Read the user's provided option and the accompanied arguments
if [[ $# -gt 0 ]]
then
    while getopts "s:b:l" opt
    do
        case $opt in
            s) option_s=1; input_option="s"
            option_s_input=$2
            if [ -z $option_s_input ] || [[ $option_s_input =~ ^[[:punct:][:space:]]{0,}$ ]] # Exit when no provided argument or argument has only special characters
            then
                printf "%s\n" "$OPTERR"
                exit 0
            fi
            ;;
            b) option_b=1; input_option="b"
            option_b_input=$2
            if [ -z $option_b_input ]
            then
                printf "%s\n" "$OPTERR"
                exit 0
            fi
            ;;
            l) option_l=1; input_option="l"
            ;;
            *)
            echo -e "$OPTERR"
            exit 0
            ;;
        esac
    done
else
    printf "%s\n" "${RED}No option/arg(s) provided. Exiting...${NC}"
    exit 0
fi

# Check multiple options
multiple_options_validate

# Get provided option and run the corresponding functions/operations
case $input_option in
    s) option_s_operation $option_s_input
    ;;
    b) get_option_b_args $option_b_input
    option_b_operation "${option_b_args[0]}" "${option_b_args[1]}"
    ;;
    l) option_l_operation
    ;;
    *)
    echo -e "$OPTERR"
    exit 0
    ;;
esac

exit 0
