#/bin/sh

func()
{
	for file in $1
	do
		if [ -d $file ];then 
			echo $file
			cd $file
			rm -rf .cproject  .project
			func "`ls`"
			cd ..
		fi
	done
}

filelist=`ls`

func "$filelist"

