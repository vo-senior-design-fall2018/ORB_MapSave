#! /bin/bash

if [ "$1" == "" ]; then
    echo "Need folder path"
fi

for file in "$1"; do
	if [ -f "$file" ]; then
		ffmpeg -i $file -vf scale=640:-1 ../scale/$file
	fi
done
