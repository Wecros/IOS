#!/bin/sh

# @file dirgraph.sh
# @author Marek Filip (xfilip46), FIT BUT
# @date 25/Mar/2020
# @brief IOS-projekt-1
# @details Script recursively searches current repository and prints out
#          number of file and directory occurences. It also prints
#          histogram specifying the file sizes.
#          The script is POSIX compliant.

export POSIXLY_CORRECT=yes  # to ensure posix correct script

# Print x hashes dependant on the count of passed argument.
printhash() {
  total=$1 # passed count of files
  count=$total # count of hashes to be printed

  # if the '-n' flag is active
  if [ "$use_normalization" ]; then
    OFFSET=13 # offset for correct formatting
    terminal_width=$(tput cols)
    max_width=$((terminal_width - OFFSET)) # max width of line
    if [ ! -t 1 ]; then max_width=$((79 - OFFSET + 1)); fi
    # ratio of largest count of files to width of line
    ratio=$(echo "scale=2; $max_count / $max_width" | bc) 

    # if count of files is the largest of the counts and is over max width
    if [ "$total" -eq "$max_count" ] && [ "$total" -gt "$max_width" ]; then
      count=$max_width
    elif [ "$max_width" -gt "$total" ]; then
      count=$(echo "$total / $ratio" | bc)
    fi
  fi

  # check if count isn't zero to catch some edge cases for seq on Eva server
  if [ "$count" -ne 0 ]; then
    # for loop for printing hashes
    for _ in $(seq 1 "$count"); do
      printf "#"
    done
  fi
  printf "\n"
}

# Exit the program with code 1 and print message passed as parameter/
errexit() {
  echo "$1" >&2
  echo "Terminating program..." >&2
  exit 1
}

wrong_perms_echo() {
  echo wc: "‘""$1""‘": Permission denied. >&2
  wrong_perms=true
}

# Print the result of the search in the correct format
print_output() {
  echo Root directory: "$DIR"
  echo Directories: "$ND"
  echo All files: "$NF"
  echo File size histogram:
  printf "  <100 B  : " 
  printhash "$lt100B"
  printf "  <1 KiB  : " 
  printhash "$lt1KiB"
  printf "  <10 KiB : " 
  printhash "$lt10KiB"
  printf "  <100 KiB: " 
  printhash "$lt100KiB"
  printf "  <1 MiB  : " 
  printhash "$lt1MiB"
  printf "  <10 MiB : " 
  printhash "$lt10MiB"
  printf "  <100 MiB: " 
  printhash "$lt100MiB"
  printf "  <1 GiB  : " 
  printhash "$lt1GiB"
  printf "  >=1 GiB : " 
  printhash "$ge1GiB"
}

# Process file, increment file count, compute size and increment correct var
process_file() {
  # compute the size of file, terminate if insufficient permissions
  size=$(wc -c 2>/dev/null < "$1") || { wrong_perms_echo "$file"; return 1; }
  NF=$((NF+1)) # increment file count
  # increment the variable count according to size of file
  if [ "$size" -lt 100 ]; then
    lt100B=$((lt100B+1))
  elif [ "$size" -lt 1024 ]; then
    lt1KiB=$((lt1KiB+1))
  elif [ "$size" -lt 10240 ]; then
    lt10KiB=$((lt10KiB+1))
  elif [ "$size" -lt 100240 ]; then
    lt100KiB=$((lt100KiB+1))
  elif [ "$size" -lt 1048576 ]; then
    lt1MiB=$((lt1MiB+1))
  elif [ "$size" -lt 10485760 ]; then
    lt10MiB=$((lt10MiB+1))
  elif [ "$size" -lt 104857600 ]; then
    lt100MiB=$((lt100MiB+1))
  elif [ "$size" -lt 1073741824 ]; then
    lt1GiB=$((lt1GiB+1))
  else
    ge1GiB=$((ge1GiB+1))
  fi
}

# Process dir, increment directory count.
process_dir() {
  ND=$((ND+1))
}

# dirgraph [-i FILE_ERE] [-n] [DIR], parsing arguments here
while getopts ":i:n" opt; do
  case ${opt} in
    i )
      FILE_ERE=$OPTARG
      ;;
    n )
      use_normalization=true
      ;;
    \? )
      errexit "Invalid option: $OPTARG."
      ;;
    : )
      errexit "Invalid option: $OPTARG requires an argument."
      ;;
  esac
done
shift $((OPTIND -1))

if [ "$1" ]; then  # if argument exists
  # change to the root's directory, if not found, exit with error code.
  DIR="$1"
  cd "$1" 2>/dev/null || errexit "Specified directory does not exist or cannot be accessed."
else
  DIR=$(pwd)
fi
ROOT=$(pwd) # assign root to current working directory
ND=0 # directory count
NF=0 # file count
# count of files for different file sizes
lt100B=0
lt1KiB=0
lt10KiB=0
lt100KiB=0
lt1MiB=0
lt10MiB=0
lt100MiB=0
lt1GiB=0
ge1GiB=0

# search the files recursively, if user has insufficient permissions: set flag
if [ -z "$FILE_ERE" ]; then
  files=$(find "$ROOT" -type f -name "*") || wrong_perms=true
  dirs=$(find "$ROOT" -type d -name "*") || wrong_perms=true
else
  # grep is used in braces here because we want to check the return code of find
  files=$(find "$ROOT" -type f -name "*" | grep -vE "$FILE_ERE") || wrong_perms=true
  dirs=$(find "$ROOT" -type d -name "*" | grep -vE "$FILE_ERE") || wrong_perms=true
fi

# Set the internal field seperator for for loops as newline
IFS="
"
for file in $files; do
  process_file "$file"
done
for dir in $dirs; do
  process_dir "$dir"
done

# if madness to determine which count of the files is largest
if [ "$use_normalization" = true ]; then
  max_count=$lt100B
  if [ "$lt1KiB" -gt "$max_count" ]; then
    max_count=$lt1KiB
  fi
  if [ "$lt10KiB" -gt "$max_count" ]; then
    max_count=$lt10KiB
  fi
  if [ "$lt100KiB" -gt "$max_count" ]; then
    max_count=$lt100KiB
  fi
  if [ "$lt1MiB" -gt "$max_count" ]; then
    max_count=$lt1MiB
  fi
  if [ "$lt10MiB" -gt "$max_count" ]; then
    max_count=$lt10MiB
  fi
  if [ "$lt100MiB" -gt "$max_count" ]; then
    max_count=$lt100MiB
  fi
  if [ "$lt1GiB" -gt "$max_count" ]; then
    max_count=$lt1GiB
  fi
  if [ "$ge1GiB" -gt "$max_count" ]; then
    max_count=$ge1GiB
  fi
fi

 # to catch some edge cases when regexing the basename
if [ "$ND" -eq 0 ]; then
  ND=1
fi
# if user had insufficient permissions for viewing some files
if [ "$wrong_perms" ]; then
  echo Not all files were counted because of insufficient permissions! >&2
fi

print_output # print the results in the correct format
# exit the program with exit code 1 if the user had insufficient permissions
if [ "$wrong_perms" ]; then
  exit 1
fi
