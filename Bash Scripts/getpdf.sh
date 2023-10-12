#!/bin/bash

# This script downloads all the pdfs in a website and saved it in a new subdirectory in the same
# directory as this script. The users have the option to compress the pdfs subdirectory in the 
# command line. The pdfs directory is named using this format "pdfs_YYYY_MM_DD_HH_MM_SS". The script
# prompts an input line to allow users enter the URL link to the website containing the pdf document links.

# Usage example:
#  ./getpdf.sh => This command just saves the pdfs to a directory
#  ./getpdf.sh -z => This command saves the pdfs to a directory and compresses the directory

# Declare the output text color variables
BLUE=$'\033[0;34m'
GREEN=$'\033[0;32m'
RED=$'\033[0;31m'
NC=$'\033[0m'

# Error message output by getopts command
OPTERR="${RED}Invalid flag in usage. Exiting ..${NC}"

# Check whether user provide a option when running the script
if [[ $# -gt 0 ]]
then
    # When user use option z set z_flag variable to true
    # Any other options prompts error message and end script
    while getopts "z" opt
    do
        case $opt in
            z) z_flag=true
            ;;
            *) echo -e "$OPTERR" && exit 0;
            ;;
        esac
    done
fi

# A function handles getting pdf hyperlinks, downloading the pdfs and counting the downloaded pdfs
download_pdfs()
{
    printf "Downloading in process "
    pdfs_links=$(curl -s $input_url | grep -Eo '(http|https)://[^"]+' | grep -o ".*.pdf")
    for hyperlink in $pdfs_links
    do
        wget -P $1 $hyperlink -q
        printf ". "
    done
    # Create a new line to print more message
    printf "\n"
    # Get the total number of downloaded pdf items
    downloaded_count=$(ls $1| wc -l)
    # Print downloading message and the total downloaded pdf items
    printf "Downloading . . . . . . ${GREEN}$downloaded_count${NC} PDF files have been downloaded to the directory ${GREEN}$1${NC}\n"
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

# A function to get and print pdfs name and size
get_pdfs_name_size()
{
    # Print the table header
    printf "${BLUE}%-20s  |   %-10s${NC}\n" "File Name" "Size(Bytes)"
    # Iterate through each item in the directory
    for item in $1/*
    do
        # Check whether the item is a file or not
        if [[ -f $item ]]
        then
            # Extract item's basename and assign to var
            item_name=$(basename $item)
            # Get the item's size and convert it to the biggest unit and print the name and size in terminal
            item_size=$(interpret_size $(du -b $item | cut -f 1))
            printf "%-20s  ${BLUE}|${NC}   %-10s\n" $item_name $item_size
        fi
    done
}

# A function to compress the pdfs directory and prints the pdfs archive name and directory path
compress_dir()
{
    if [[ $z_flag ]]  # Check whether user use -z flag
    then
        # Compress the downloaded pdfs directory and print the result out in the terminal
        archive_name=$1".zip"   # Assign archive's name to a var
        cd ./$1 # Change pwd to pdfs directory
        zip -r "./$archive_name" ./* -qq # Run zip command
        cd ../  # Change pwd back to the original directory
        printf "PDFs archived to ${GREEN}$archive_name${NC} in the ${GREEN}$1${NC} directory.\n"
    fi
}

# Assign user input URL to a variable
read -p "Enter a URL: " input_url

# Count available pdf links in the URL's webpage
output_length=$(curl -s $input_url | grep -Eo '(http|https)://[^"]+' | grep ".pdf" | sed "s/.*\///g" | wc -l)

# Check whether the URL webpage has pdf links or not
if [ $output_length -gt 0 ]
then
    # Create a directory to store the pdfs
    dir_name="pdfs_"$(date +%Y_%m_%d_%H_%M_%S)
    mkdir $dir_name
    # Download the pdfs and save it to the targeted directory
    download_pdfs $dir_name
    # Print out the pdfs name and size
    get_pdfs_name_size $dir_name
    # Compress the pdfs into an archive
    compress_dir $dir_name
else
    echo -e "${RED}No PDFs found at this URL - exiting..${NC}"
    exit 0
fi

exit 0