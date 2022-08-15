#! /bin/bash
#定义函数，该函数用来判断源目录中的文件是否在目标文件
function in_tar(){
    #保存传入的参数和变量
	local x=$1
	local n=$2
	local i
	local dir_t=$3
    #遍历传入的文件
	for i in ${dir_t[*]}
	do
        #一旦找到就退出
		if [ "$x" = "$i" ];then
			return 1
		fi
	done
    #否则返回0
	return 0
}
#该函数用于判断给定文件是否在源文件目录中
function in_sou(){
	#保存变量
	local x=$1
	local n=$2
	local i
	dir_t=$3
	#遍历目录
	for i in ${dir_t[*]}
	do
		if [ "$x" = "$i" ];then
            #找到后返回1
			return 1
		fi
	done
	#没找到返回0
	return 0
}
#该函数用于比较时间，t1和t3来自一组,t2和t4来自一组
function comptime(){
	local t1=$1
	local t2=$2
	local t3=$3
	local t4=$4
	#下面用于比较两个时间哪个在前
	if (($t1 < $t3));then
		#"返回2"
		return 2
	elif (($t1 > $t3));then
		#"返回1"
		return 1
	else
		if (($t2 == $t4));then
			# "返回0"
			return 0
		elif (($t2 > $t4));then
			# "返回1"
			return 1
		else
			#"返回2"
			return 2
		fi
	fi
}
#执行主体
function dirsync(){
#定义一些有用的变量
local -a dir_tar
local -a dir_sou
local -i tar_len
local -i sou_len
local -a tar_time
local -a sou_time
local i
local tar=$2
local sour=$1
#查看参数传入的个数是否正确
if [ $# -ne 2 ];then
	echo "参数的个数不是两个"
else
	echo "$1 $2"
	if [[ ! -d $1 || ! -d $2 ]];then
		echo "输入的不是目录"
		exit 1
	fi
	if [[ $1 = $2 ]];then
		echo "备份两个相同的目录"
		exit 0
	fi
	#设置变量
	set $(ls $tar)
    #目录内容与长度
	dir_tar=("$@")
	tar_len=$#
	set $(ls $sour)
    #目录内容与长度
	dir_sou=("$@")
	sou_len=$#
    #遍历目标文件夹中的文件
	for i in ${dir_tar[@]}
	do
		#看他是否在源目录中
		in_sou $i $sou_len "${dir_sou[*]}"
		temp=$?
		if [ $temp -eq 0 ];then
			#"这个没有找到，直接删除该目录
			if rm -rf $tar/$i;then
				echo "删除$tar/$i"
			fi
		fi
	done
    #重新设置变量和长度
	set $(ls $tar)
	dir_tar=("$@")
	tar_len=$#
    #遍历源目录
	for i in ${dir_sou[@]}
	do
		in_tar $i $tar_len "${dir_tar[*]}"
		temp=$?
		#查看当前文件是否出现在目标目录中
		if [ $temp -eq 0 ];then
			#这个没有找到，则直接复制拷贝过去
			if cp -r -p $sour/$i $tar;then
				echo "备份$sour/$i至$tar"
			fi
		fi
	done
	#重新遍历源目录
	for i in ${dir_sou[@]}
	do
		#对于源目录中的文件夹
		if [ -d $sour/$i ];then
        #获取创造时间
			set -- $(stat --format=%y $tar/$i)
			tar_time[0]="${1:0:4}""${1:5:2}""${1:8:2}"
			tar_time[1]="${2:0:2}""${2:3:2}""${2:6:2}"
			set -- $(stat --format=%y $sour/$i)
			sou_time[0]="${1:0:4}""${1:5:2}""${1:8:2}"
			sou_time[1]="${2:0:2}""${2:3:2}""${2:6:2}"
		#比较这个文件在两个文件夹中的事件
			comptime ${tar_time[0]} ${tar_time[1]} ${sou_time[0]} ${sou_time[1]}
			if [ $? -eq 0 ];then
				#"时间相同不处理" 
				continue
			else
                #否则递归调用这两个文件夹中的内容，对其进行备份处理
				#echo "递归调用dirsync  $sour/$i $tar/$i"
				dirsync  $sour/$i $tar/$i
				#echo "结束调用dirsync  $sour/$i $tar/$i"
			fi
		else
            #对于非目录而言，获取时间
			set -- $(stat --format=%y $tar/$i)
			tar_time[0]="${1:0:4}""${1:5:2}""${1:8:2}"
			tar_time[1]="${2:0:2}""${2:3:2}""${2:6:2}"
			set -- $(stat --format=%y $sour/$i)
			sou_time[0]="${1:0:4}""${1:5:2}""${1:8:2}"
			sou_time[1]="${2:0:2}""${2:3:2}""${2:6:2}"
			#比较时间
			comptime ${tar_time[0]} ${tar_time[1]} ${sou_time[0]} ${sou_time[1]}
			temp=$?
            #根据时间做处理
			if [ $temp -eq 0 ];then
				#"时间相同不处理" 
				continue
			elif [ $temp -eq 2 ];then
                #时间不同则相互更新
				if cp -r -p $sour/$i $tar;then
					echo "用$sour/$i更新$tar"
				fi
			else
                #时间不同相互更新
				if cp -r -p $tar/$i $sour;then
					echo "用$tar/$i更新$sour"
				fi
			fi
		fi
	done
	
fi
}
#调用该程序
dirsync $1 $2