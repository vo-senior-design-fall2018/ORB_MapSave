echo
echo "This program processes video for ORBSLAM2"
echo

echo -n "Write the name of the video file in this folder: "
read inputname
echo

if [ ! -e "$inputname" ]
then
	echo "File not found. Make sure it is in the same folder as VideoProcessor.sh"
	echo
	exit 1
fi

echo -n "What would you like the output file to be called: "
read outputname
echo

mkdir -p $outputname/rgb

echo -n "Choose frames per second. Between 10 and 25 is preferable: "
read fps
echo

echo "The video must be wider than it is long"
echo -n "Would you like to rotate it (y/n): "
read rotchoice
echo

if [ $rotchoice = "n" ] || [ $rotchoice = "N" ]
then
	ffmpeg -i $inputname -r $fps -vf scale=-1:640 $outputname/rgb/img%04d.png

elif [ $rotchoice = "y" ] || [ $rotchoice = "Y" ]
then
	ffmpeg -i $inputname -r $fps -vf scale=320:-1,"transpose=1" $outputname/rgb/img%04d.png

	```
else
	echo "Invalid choice. Choose y/n"
	echo
	exit 1
	```

fi

#Counts the number of output files
imgnum=$(ls $outputname/rgb | wc -l)

echo "# colour images" > $outputname/rgb.txt
echo "#file: '$outputname'" >> $outputname/rgb.txt
echo "# timestamp filename" >> $outputname/rgb.txt

#Uses bc to calculate timestamp increment to 6 places
#No spaces around =
frameTime=$(bc <<< "scale=6; 1.0/$fps")
timestamp=0.000000

for i in $(seq -f "%04g" $imgnum)
do
	echo $timestamp rgb/img$i.png >> $outputname/rgb.txt
	timestamp=$(bc <<< "scale=6; $timestamp+$frameTime")
done

mv $inputname $outputname

echo
echo "Your files are ready, and have all been put in a single folder."
echo "Please place this folder in ~/Desktop/ORBSLAM2 datasets/our datasets."
echo
