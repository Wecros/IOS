#!/bin/sh
export POSIXLY_CORRECT=yes  # to ensure posix correct script

# dirgraph [-i FILE_ERE] [-n] [DIR]
while getopts ":i:n" opt; do
  case ${opt} in
    i )
      FILE_ERE=$OPTARG
      ;;
    n )
      echo n called
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

ROOT=$1
count=0

for i in $(cd "$ROOT" || exit 1; find *); do
  count=$((count+1))
done


echo Root directory: "$ROOT"
echo Directories: 
echo All Files: $count
echo File size histogram:

