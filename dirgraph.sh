#!/bin/sh
export POSIXLY_CORRECT=yes  # to ensure posix correct script

# Print x hashes dependant on the count of passed argument
printhash() {
  total=$1 # passed count of files
  count=$total # count of hashes to be printed

  # if the '-n' flag is active
  if [ "$use_normalization" = true ]; then
    OFFSET=13 # offset for correct formatting
    terminal_width=$(tput cols)
    max_width=$((terminal_width - OFFSET)) # max width of line
    # ratio of largest count of files to width of line
    ratio=$(echo "scale=2; $max_count / $max_width" | bc) 

    # if count of files is the largest of the counts and is over max width
    if [ "$total" -eq "$max_count" ] && [ "$total" -gt "$max_width" ]; then
      count=$max_width
    elif [ "$total" -gt "$max_width" ]; then
      count=$(echo "$total / $ratio" | bc)
    fi
  fi

  # for loop for printing hashes
  for ((; count > 0; count--)); do
    printf "#"
  done
  printf "\n"
}

# dirgraph [-i FILE_ERE] [-n] [DIR]
while getopts ":i:n" opt; do
  case ${opt} in
    i )
      FILE_ERE=$OPTARG
      ;;
    n )
      use_normalization=true
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      ;;
  esac
done
shift $((OPTIND -1))

ROOT=$1 # passed argument of folder to search
NF=0 # file count
ND=0 # directory count
lt100B=0
lt1KiB=0
lt10KiB=0
lt100KiB=0
lt1MiB=0
lt10MiB=0
lt100MiB=0
lt1GiB=0
ge1GiB=0

# change to the root's directory, if not found, exit with error code.
cd "$ROOT" || exit 1;

if [ -z "$FILE_ERE" ]; then
  find=$(find *)
else
  find=$(find * | grep -v -E "$FILE_ERE")
fi

# for each file in found files
for i in eval $find; do
  if [ -d $i ]; then
    ND=$((ND+1))
  elif [ -f $i ]; then
    NF=$((NF+1))
    size=$(wc -c < $i)

    if [ "$size" -lt 100 ]; then
      lt100B=$((lt100B+1))
    elif [ "$size" -lt 1000 ]; then
      lt1KiB=$((lt1KiB+1))
    elif [ "$size" -lt 10000 ]; then
      lt10KiB=$((lt10KiB+1))
    elif [ "$size" -lt 100000 ]; then
      lt100KiB=$((lt100KiB+1))
    elif [ "$size" -lt 1000000 ]; then
      lt1MiB=$((lt1MiB+1))
    elif [ "$size" -lt 10000000 ]; then
      lt10MiB=$((lt10MiB+1))
    elif [ "$size" -lt 100000000 ]; then
      lt100MiB=$((lt100MiB+1))
    elif [ "$size" -lt 1000000000 ]; then
      lt1GiB=$((lt1GiB+1))
    else
      ge1GiB=$((ge1GiB+1))
    fi
  fi
done

echo ROOT directory: "$ROOT"
echo Directories: $ND
echo All Files: $NF
echo File size histogram:

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


printf "  <100B   : " 
printhash "$lt100B"
printf "  <1KiB   : " 
printhash "$lt1KiB"
printf "  <10KiB  : " 
printhash "$lt10KiB"
printf "  <100KiB : " 
printhash "$lt100KiB"
printf "  <1MiB   : " 
printhash "$lt1MiB"
printf "  <10MiB  : " 
printhash "$lt10MiB"
printf "  <100MiB : " 
printhash "$lt100MiB"
printf "  <1GiB   : " 
printhash "$lt1GiB"
printf "  >=1GiB  : " 
printhash "$ge1GiB"
