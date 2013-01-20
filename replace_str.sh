#!/bin/sh

#sed 's/global/crawler/g' $1 > $1.tmp
#mv $1.tmp $1
files=`find . -name '*.cc'`
for file in $files
do
    echo $file
    sed -i 's/global/crawler/g' $file
done

files=`find . -name "*.h"`
for file in $files
do
    echo $file
    sed -i 's/global/crawler/g' $file
done

#使用vim替换命令
#日期：2012.04.19
#功能描述：在FIND_PATH路径下所有名为FILE_NAME的文件内容中，将SOURCE串替换为DEST串
#比如下例：将/home/zxx/文档/1路径下所有名为makefile的文件，凡是文件内容中含有-Werror的，都替换为空

##---------------------下面是你可以修改的参数------------------
##替换的文件都在这个路径下面
#FIND_PATH="/home/zxx/文档/1"
##替换的文件，替换所有文件指定为""
#FILE_NAME="makefile"
##要替换的串
#SOURCE="global"
##目的串
#DEST="crawler"
##--------------------上面是你可以修改的参数--------------------
#
#makefilepath=$(find $FIND_PATH -name $FILE_NAME)
#for way in $makefilepath
#do
#    echo $way
##-!代表下面内容是输入，而不从键盘输入
##要注意下面的$前面要加上\，否则会被解析为变量
#    vim -e $way<<-!
#    :1,\$s/$SOURCE/$DEST/g
#    :wq
#    !
#done
