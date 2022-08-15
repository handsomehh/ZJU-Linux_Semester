#! /bin/bash 
###
# @Author: 韩恺荣
# @Date: 2022-08-08 11:28:36
 # @LastEditTime: 2022-08-14 11:57:51
 # @LastEditors: your name
# @Description: 实现了简易版的vim
###
#定义全局变量

declare -x filename #打开的目标文本名称
declare -x row #打开的目标文本行数
declare -x content #内存存储在该变量

declare -x width #屏幕的宽度
declare -x height #屏幕的高度
declare -x mode #当前的模式，0是normal，1是insert，2是 visual（cmd模式）

declare -x begin_offset #屏幕中的文本开头偏移量
declare -x end_offset #屏幕文本结束位置
declare -x offset #文本总的偏移
declare -x cur_offset #当前光标所在的偏移量

declare -x mouse_x #光标的x坐标
declare -x mouse_y #光标的y坐标

declare -x tty_row #屏幕的行数
declare -x tty_clo #屏幕的列数

declare -xai screen_rowlen #屏幕中的每一行的字符数
declare -x cmd_input #cmd模式下的输入
declare -x row_occupy #屏幕中显示了多少行
declare -x delete_mask #删除时的目标

#----------------------------
#remind函数打印屏幕中的提示信息
#----------------------------
function remind(){
	#保存传入参数
	local re="$1"
	#提取屏幕的大小 
	width=$(stty size | awk '{print $2}')
	height=$(stty size | awk '{print $1}')
	#清屏
	clear
	#打印鼠标位置
	printf '\033[%d;%dH' $height 1
	#判断mode
	case $mode in
	0) #normal
		string="\"$filename\""" : normal"
		Cal_cur_offset
		
		local percent
		if ((offset==0));then
			percent=0
		else
			percent=$((100*cur_offset/offset))
		fi
		#打印鼠标位置，以及当前阅读的百分比
		printf ">>$string  X:$mouse_x Y:$mouse_y scale:$percent %%"
	;;
	1) #insert
		string="\"$filename\""" : insert"
		#打印鼠标位置
		printf ">>$string  X:$mouse_x Y:$mouse_y"
	
	;;
	2) #visual
		#string="\"$filename\""":visual"
		printf ">> :$re"
	;;
	esac
}

#-----------------------------------------------
#Cal_cur_offset函数计算当前鼠标位置所在的文本偏移量
#----------------------------------------------
function Cal_cur_offset(){
	sum=0
	#遍历每一行
	for ((i=1;i<=$mouse_y-1;i++))
	do
	 	((sum+=${screen_rowlen[$i]}))
	done
	#加上当前的位置
	cur_offset=$(($sum+$mouse_x+$begin_offset-1))
}

#---------------------------------------
#is_n函数判断当前位置的前一个字符是不是回车
#---------------------------------------
function is_n(){
	Cal_cur_offset
	#当是回车时，返回值为1
	if [[ ${content:$(($cur_offset-1)):1} = $'\n' ]];then
		return 1
	else
	#否则返回0
		return 0
	fi
}

#---------------------------------------
#Back_n函数，用于文本回退时，判断是否需要
#重开一行，即如果当前光标在（1，1），假设
#前面还有文本，在回退时要将前面的文本回滚
#回来，但此时的鼠标要跟在上一行的末尾，因
#此该函数是保证格式正确的必要函数
#---------------------------------------
function Back_n(){
	#当开始的偏移量是0时，什么都不做
	if (($begin_offset<=0));then
		return 0	
	fi
	#printf "$begin_offset"
	#否则去找他上一个行的开始字符
	local pos=$(($begin_offset-1))
	while true
	do
		#直到找到上一行的开始标志
		if (($pos<=0)) || [[ ${content:$(($pos-1)):1} = $'\n' ]];then
			break
		fi
		#每次位置向前推移一个
		((pos--))
	done
	#printf "mouse_x=$begin_offset-$pos yu $width -1"
	#此时的pos恰好指向上一行的最后一个字符
	if (($(($begin_offset-$pos))%$width == 0));then
	#当中间的字符数量能被宽度整除
		if [[ ${content:$((begin_offset-1)):1} = $'\n' ]];then
		#上一个字符恰好是回车时
			mouse_x=$((width-1))
			begin_offset=$(($begin_offset-$width))
		else
		#当上一个字符不是回车时
			mouse_x=$width
			begin_offset=$(($begin_offset-$width))
		fi
	elif (($(($begin_offset-$pos))%$width == 1));then
	#当余1时
		if (( $(($begin_offset-$pos)) == 1 ));then
		#有可能前面的字符只有一个
			begin_offset=$((begin_offset-1))
			mouse_x=1
		else 
		#前面的字符有多个
			mouse_x=$width
			begin_offset=$(($begin_offset-$width-1))
		fi
	else
	#对于一般情况
		mouse_x=$(($(($begin_offset-$pos))%$width-1))
		begin_offset=$(($begin_offset-$mouse_x-1))
	fi
	#保证光标的位置非0
	if (($mouse_x == 0));then
		mouse_x=1
	fi
}

#---------------------------------------
#normal_deal函数，处理normal模式下的键盘事件
#---------------------------------------
function normal_deal(){
	#保存参数
	input="$1"
	#计算当前便宜
	Cal_cur_offset

	case "$input" in
	i) 
	#输入i，跳转插入模式
		mode=1
		;;
	^)
	#输入^，回到行首
		mouse_x=1
		;;
	h|$'\E[D')
	#光标左移
		if (($mouse_x > 1));then
		#一般情况下，直接x--
			((mouse_x--))
		else
		#当到达每一行的第一个字符时，光标左移需要考虑换行问题
			if (($mouse_y == 1));then
				Back_n
			elif (($mouse_y>1));then
			#当是在屏幕中间的行时
				if ((${screen_rowlen[$mouse_y-1]} == $width));then
				#当他的上一行满了时
					if Is_n ;then
					#如果上一行最后一个是回车
						mouse_x=$((${screen_rowlen[$mouse_y-1]}-1))
						#则mouse_x=边界宽减1
						if (($mouse_x==0));then
							mouse_x=1
						fi
						((mouse_y--))
					else
					#如果不是回车
						mouse_x=$((${screen_rowlen[$mouse_y-1]}))
						#等于边界宽
						if (($mouse_x==0));then
							mouse_x=1
						fi
						#y--
						((mouse_y--))
					fi
				else
				#当上一行没满时
					mouse_x=$((${screen_rowlen[$mouse_y-1]}-1))
					#之间等于边界宽减1
					if (($mouse_x==0));then
						mouse_x=1
					fi
					#y--
					((mouse_y--))
				fi
			fi
			#保证光标的位置始终大于0
			if (($mouse_x==0));then
				mouse_x=1
			fi
		fi
		;;
        j|$'\E[B'|$'\n')
        #当向下一移动光标时
		if ((mouse_y<$height-2))&&((mouse_y<row_occupy));then
		#如果在屏幕中间或者是在小于结尾处
			((mouse_y++))
			#判断当前的光标是否在下一行末尾的后面
			if (( $mouse_x >= ${screen_rowlen[$mouse_y]}-1 ));then
				mouse_x=$((${screen_rowlen[$mouse_y]}-1))
			fi
		#如果达到了最后一行
		elif ((mouse_y==$height-2));then
			num=0
			flag=0
		#从当前的位置一直排查到最后一个位置
			for ((i=1;i<=$((offset-end_offset-1));i++))
			do
			#每排查一个位置num++
				((num++))
				#当找到有回车或者位置的数量大于屏幕的宽度时
				if [[ ${content:$((end_offset+i)):1} = $'\n' ]] || (($num >= $width));then
					flag=1
					break
				fi
			done
			#如果可以参与显示
			if ((flag == 1));then
				((begin_offset+=${screen_rowlen[1]}))
				remind ""
			display_content $begin_offset
			printf '\033[%d;%dH' $mouse_y $mouse_x
				mouse_x=1
			fi
		else
		#如果到了屏幕中的非最后一行的末尾位置
			:
		fi
		#保证mouse_x始终大于0
		if (($mouse_x==0));then
		mouse_x=1
	fi
			;;
	k|$'\E[A')
	#向上翻页
		if ((mouse_y>1));then
		#屏幕中间时，直接y--
			((mouse_y--))
			#保证格子不会不溢出
			if (( $mouse_x >= ${screen_rowlen[$mouse_y]}-1 ));then
				mouse_x=$((${screen_rowlen[$mouse_y]}-1))
			fi
		else
		#在第一行时，调用back_n
			Back_n
		fi
		#保证x大于等于1
		if (($mouse_x==0));then
		mouse_x=1
	fi
			;;
	l| $'\E[C')
		#计算当前的位移量
		Cal_cur_offset
		#当前位移量大于或等于总偏移量时，无视本次操作
		if (($cur_offset>=$offset));then
			return 0
		fi
		#当鼠标x坐标小于当前行的最大字母数时
		if (($mouse_x < ${screen_rowlen[$mouse_y]}-1));then
			#向右偏移一位
			((mouse_x++))
		else
		#当鼠标大于等于最大字母数时
			if (($mouse_y == $height - 2));then
			#当y坐标达到底部时，判断是否需要换行，处理结尾偏移量
				if (($mouse_x == ${screen_rowlen[$mouse_y]}-1));then
					if [[ ${content:$((cur_offset+1)):1} = $'\n' ]];then
					#换行时保证begin offset的正确
						((begin_offset+=${screen_rowlen[1]}))
						mouse_x=1
					else
					#x++
						((mouse_x++))
					fi
				else
					if ((cur_offset == offset - 1));then
					#当达到末尾，需要实现如下的情况： 字符1 字符2 光标，以实现结尾的追加
						if (($mouse_x < $width));then
						#不需要换行时
							((mouse_x++))
						else
						#需要换行时
							((begin_offset+=${screen_rowlen[1]}))
							mouse_x=1
							#((mouse_y++))
						fi
					else
						((begin_offset+=${screen_rowlen[1]}))
						mouse_x=1
					fi
				fi
			else 
			#当还在屏幕中间时,且达到文本的最后一行
				if (( mouse_y == row_occupy));then
				#若在屏幕的最后一行，因为最后一行的单词个数不用考虑最后的换行问题
				
					if (($mouse_x == ${screen_rowlen[$mouse_y]}-1));then
					#所以当x坐标是最大位移量-1时直接++
						((mouse_x++))
					elif (($mouse_x == ${screen_rowlen[$mouse_y]}));then
					#否则判断是否另起一行
						if (($mouse_x < $width));then
							((mouse_x++))
						else
							mouse_x=1
							((mouse_y++))
						fi
					#处理后达到offset位置
					fi
					
				else
			#当还在屏幕中间时的一般情况
					if (($mouse_x == ${screen_rowlen[$mouse_y]}-1));then	
						if [[ ${content:$(($cur_offset+1)):1} = $'\n' ]];then
						#当遇到回车
							mouse_x=1
							((mouse_y++))
						else
						#当一般情况
							((mouse_x++))
						fi	
					else
					#当到达最大偏移量
						mouse_x=1
						((mouse_y++))
					fi
				fi
			fi
		fi
		#显示文本
		remind ""
		#更新每一行的状态
		display_content $begin_offset
		#鼠标更新
		printf '\033[%d;%dH' $mouse_y $mouse_x
		;;
	:)
	#输入：时，进入visual（cmd）模式
		mode=2
	;;
	esac
	#更新实时位置
	Cal_cur_offset
}

#---------------------------------------
#display_content函数，显示所有文本，并跟新
#一些状态
#---------------------------------------
function display_content(){
	#将光标指向1,1
	printf '\033[%d;%dH' 1 1
	#保存offset
	local store=$1
	local begin_offset=$1
	#默认行初始值为1
	row_occupy=1
	local clo_occupy=0
	#默认列初始值为0
	#初始化每行的存储列表
	for ((i=1;i<=$height - 2;i++))
	do
		screen_rowlen[$i]=0
	done
	#记录回车数
	num=0
	local flag=0
	#开始显示与渲染
	while true
	do
		#当前展示的数为最后一个数时
		if (( $begin_offset == $offset));then
		#结束符号是最后一个数
			end_offset=$((offset-1))
			#记录当前行的列数
			screen_rowlen[$row_occupy]=$clo_occupy
			flag=1
			break
		fi
		#当前展示为回车时
		if [[ ${content:$begin_offset:1} = $'\n' ]];then
			((num++))
			#该行的字符数加一
			screen_rowlen[$row_occupy]=$(($clo_occupy+1))
			clo_occupy=0
			#行数加一
			((row_occupy++))
			end_offset=$begin_offset
		elif (( $clo_occupy == $width));then
		#当前行满时
			screen_rowlen[$row_occupy]=$clo_occupy
			clo_occupy=1
			((row_occupy++))
			end_offset=$(($begin_offset-1))
		else
		#普通字符的展示直接加加
			((clo_occupy++))
		fi
		#如果屏幕已经占满
		if (($row_occupy > $height - 2));then
			((row_occupy--))
			break
		fi
		#printf "${content:$begin_offset:1}"
		#展示下一个字符
		((begin_offset++))
	done
	if ((begin_offset==0));then
	#当文本头部时
		end_offset=0
	else
	#否则不对之前得到的offset操作
		:
		#end_offset=$(($begin_offset-1))
	fi
	#打印文本
	printf "${content:$store:$(($begin_offset-$store))}"
	#下面的循环是debug时使用的信息，此处无影响
	for ((i=1;i<=$row_occupy;i++)) 
	do
		:
		#printf "\n$i:${screen_rowlen[$i]}"
	done
}

#---------------------------------------
#Init函数，初始化一些状态
#---------------------------------------
function Init(){
	#光标默认为（1，1）
	mouse_x=1
	mouse_y=1
	#计算当前的屏幕大小
	height=$(stty size | awk '{print $2}')
	width=$(stty size | awk '{print $1}')
	#计算偏移量
	Cal_cur_offset
}

#---------------------------------------
#visual_deal函数，处理cmd模式下的命令
#---------------------------------------
function visual_deal(){
	#存储传入的参数
	cmd_input="$1"
	num=0
	#当输入的是HOME键
	if [[ $cmd_input =  $'\e[H' ]];then
	#打印提示信息
		printf "Home"
		sleep 2
		#更新光标位置
		mouse_x=1
		mouse_y=1
		#更新mode和offset
		begin_offset=0
		mode=0
		#退出函数
		return 0
	fi
	#否则一直循环
	while true
	do
		#光标位置控制
		printf '\033[%d;%dH' $height 5
		printf "          "
		printf '\033[%d;%dH' $height 5
		printf "$cmd_input"
		#读取输入
		read -sN1 text
		read -sN1 -t 0.0001 k1
		read -sN1 -t 0.0001 k2
		read -sN1 -t 0.0001 k3
		#拼接字符串
		text+=${k1}${k2}${k3}
		#输入回车时
		if [[ $text = $'\n' ]];then
		#准备处理并返回normal模式
			mode=0
			break	
		elif [[ $text = $'\E' ]];then
		#输入esc，直接返回
			cmd_input=""
			mode=0
			break
		elif [[ $text = $'\E[3~' || $text = $'\177' ]];then
		#输入delete/backspace时
			if ((${#cmd_input}>0));then
			#输入的cmd命令回退一个
				cmd_input=${cmd_input:0:$((${#cmd_input}-1))}
				((num--))
			fi
		elif [[ $text =  $'\e[H' ]];then
			#打印提示信息
			printf "Home"
			sleep 2
			#更新光标位置
			mouse_x=1
			mouse_y=1
			#更新mode和offset
			begin_offset=0
			mode=0
			#退出函数
			return 0
		else
			if ((num<=10));then
			#cmd命令字符最多不超过10个
			cmd_input=${cmd_input}"$text"
			((num++))
			fi
		fi
	done
	#处理输入的cmd命令
	case "$cmd_input" in 
	"q" )
	#q代表直接退出
		clear
		exit 1
		;;
	"wq")
	#wq保存并退出
		printf "$content" > $filename
		clear
		exit 1
		;;
	"w" )
	#w保存
		printf "$content" > $filename
		return 0
		;;
	"gg")
	#gg返回第一行第一列
		mouse_x=1
		mouse_y=1
		begin_offset=0
		mode=0
		return 0
		;;
	"G")
	#G到达当前屏幕的最后一行的第一个
		mouse_y=$row_occupy
		mouse_x=1
		mode=0
		return 0
		;;
	esac
	#另存为功能w+空格+其他
	if [[ "${cmd_input:0:1}" = 'w' && ${#cmd_input} > 1 ]];then
		local target=""
		#目标路径是“其他”
		for ((i=1;i<${#cmd_input};i++))
		do
			#解析输入的字符串，忽视其中的空格，即“w 3 2.t xt”也会处理为“32.txt”
			if [[ ${cmd_input:$i:1} != ' ' ]];then
				target=$target"${cmd_input:$i:1}"
			fi
		done
		#将内容另存为target文件
		printf "$content" > $target
	fi 
}
#---------------------------------------
#insert_deal函数，处理insert模式下的命令
#---------------------------------------
function insert_deal(){
	input="$1"
	#状态机参数flag
	local flag=0
	#始终循环
	while true
	do
	#更新当前位置
	Cal_cur_offset
	case "$input" in
	$'\E')
	#当输入的是esc时
		mode=0
		return 0
		;;
	^)
	#当输入是^时，回到行首
		mouse_x=1
		;;
	$'\E[3~'|$'\177')
	#当输入delete和backspace时
		if (( cur_offset > 0))&&((offset>0));then
		#记录当前的删除位置，并准备进入状态机循环
			delete_mask=$cur_offset
			if (( cur_offset == 1 ));then
			#删除第一个字符
				flag=2
			else
			#删除普通字符
				flag=3
			fi
		fi
		;;
	$'\E[D')
	#当向左移动光标
		if (($mouse_x > 1));then
		#一般情况下直接移动
			((mouse_x--))
		else
		#当到达每一行的行首时
			if (($mouse_y == 1));then
			#需要向前翻页时，调用该函数
				Back_n
			elif (($mouse_y>1));then
			#当是在屏幕中间的行时
				if ((${screen_rowlen[$mouse_y-1]} == $width));then
				#当他的上一行满了时
					if Is_n ;then
					#如果上一行最后一个是回车
						mouse_x=$((${screen_rowlen[$mouse_y-1]}-1))
						#则mouse_x=边界宽减1
						if (($mouse_x==0));then
							mouse_x=1
						fi
						((mouse_y--))
					else
					#如果不是回车
						mouse_x=$((${screen_rowlen[$mouse_y-1]}))
						#等于边界宽
						if (($mouse_x==0));then
							mouse_x=1
						fi
						((mouse_y--))
					fi
				else
				#当上一行没满时
					mouse_x=$((${screen_rowlen[$mouse_y-1]}-1))
					#之间等于边界宽减1
					if (($mouse_x==0));then
						mouse_x=1
					fi
					#y--
					((mouse_y--))
				fi
			fi
			#保证x始终大于0
			if (($mouse_x==0));then
				mouse_x=1
			fi
		fi
		;;
        $'\E[B')
		#当输入的是下键时
		if ((mouse_y<$height-2))&&((mouse_y<row_occupy));then
		#判断当前的鼠标位置，屏幕中间时
			((mouse_y++))
			if (( $mouse_x >= ${screen_rowlen[$mouse_y]}-1 ));then
			#当鼠标的位置大于下一行的长度时
				mouse_x=$((${screen_rowlen[$mouse_y]}-1))
			fi
		elif ((mouse_y==$height-2));then
		#当是最后一行时
			num=0
			flag=0
			#循环判断是否需要换行
			for ((i=1;$((offset-end_offset-1));i++))
			do
				((num++))
				#当遇到回车时直接break，当num大于一行的长度也要break
				if [[ ${content:$((end_offset+i)):1} = $'\n' ]] || (($num >= $width));then
					flag=1
					break
				fi
			done
			#flag=1表示要换行
			if ((flag == 1));then
			#始终保证begin offset的正确
				((begin_offset+=${screen_rowlen[1]}))
				mouse_x=1
			fi
		else
		#否则什末都不干
			:
		fi
		#保证mouse_x始终大于0
		if (($mouse_x==0));then
		mouse_x=1
		fi
			;;
	$'\E[A')
	#当输入的是上建时
		if ((mouse_y>1));then
		#一般情况下直接y--即可
			((mouse_y--))
			if (( $mouse_x >= ${screen_rowlen[$mouse_y]}-1 ));then
			#当上一行的长度小于光标位置时
				mouse_x=$((${screen_rowlen[$mouse_y]}-1))
			fi
		else
		#否则要回退和翻页
			Back_n
		fi
		#保证mouse_x始终大于0
		if (($mouse_x==0));then
			mouse_x=1
		fi
			;;
	$'\E[C')
		#计算当前的位移量
		Cal_cur_offset
		#当前位移量大于或等于总偏移量时，无视本次操作
		if (($cur_offset>=$offset));then
			return 0
		fi
		#当鼠标x坐标小于当前行的最大字母数时
		if (($mouse_x < ${screen_rowlen[$mouse_y]}-1));then
			#向右偏移一位
			((mouse_x++))
		else
		#当鼠标大于等于最大字母数时
			if (($mouse_y == $height - 2));then
			#当y坐标达到底部时，判断是否需要换行，处理结尾偏移量
				if (($mouse_x == ${screen_rowlen[$mouse_y]}-1));then
				#当下一个字符是回车时
					if [[ ${content:$((cur_offset+1)):1} = $'\n' ]]||[[ ${content:$((cur_offset+1)):1} = $'\n' ]];then
					#保证begin offset始终正确9
						((begin_offset+=${screen_rowlen[1]}))
						mouse_x=1
					else
					#x++
						((mouse_x++))
					fi
				else
				#当到达最大偏移量
					if ((cur_offset == offset - 1));then
					#考虑是否要换行，大于width要换行
						if (($mouse_x < $width));then
							((mouse_x++))
						else
						#这是要换行时的操作
							((begin_offset+=${screen_rowlen[1]}))
							mouse_x=1
							#((mouse_y++))
						fi
					else
					#保证begin正确
						((begin_offset+=${screen_rowlen[1]}))
						mouse_x=1
					fi
				fi
			else 
			#当还在屏幕中间时,且达到文本的最后一行
				if (( mouse_y == row_occupy));then
				#若在屏幕的最后一行，因为最后一行的单词个数不用考虑最后的换行问题
				
					if (($mouse_x == ${screen_rowlen[$mouse_y]}-1));then
					#所以当x坐标是最大位移量-1时直接++
						((mouse_x++))
					elif (($mouse_x == ${screen_rowlen[$mouse_y]}));then
					#否则判断是否另起一行
						if (($mouse_x < $width));then
							((mouse_x++))
						else
							mouse_x=1
							((mouse_y++))
						fi
					#处理后达到offset位置
					fi
					
				else
			#当还在屏幕中间时的一般情况
					if (($mouse_x == ${screen_rowlen[$mouse_y]}-1));then	
						if [[ ${content:$(($cur_offset+1)):1} = $'\n' ]];then
						#当遇到回车
							mouse_x=1
							((mouse_y++))
						else
						#当一般情况
							((mouse_x++))
						fi	
					else
					#当到达最大偏移量
						mouse_x=1
						((mouse_y++))
					fi
				fi
			fi
		fi
		;;
	*)
	#当输入其他字符时
		if [[ ${input:0:1} = $'\n' ]] && ((cur_offset==offset )) &&((mouse_y == height-2))\
		|| ((${screen_rowlen[$mouse_y]}==width)) && ((mouse_y == height-2));then
		#当输入的是回车
			((begin_offset+=${screen_rowlen[1]}))
			((mouse_y--))
		fi
		#将所输入的字符串入我们的content变量
		content=${content:0:$cur_offset}"${input:0:1}"${content:$cur_offset}
		#更新offset参数
		offset=$(printf "$content" | wc -m)
		flag=1
		;;
	esac
	#计算偏移量
	Cal_cur_offset
	if (($flag==0));then
	#当是0时，对应一般的插入模式
		remind ""
		display_content $begin_offset
		#更新屏幕并更新鼠标位置
		printf '\033[%d;%dH' $mouse_y $mouse_x
	#读取下一个将要输入的内容
		read -sN1 input
		read -sN1 -t 0.0001 k1
		read -sN1 -t 0.0001 k2
		read -sN1 -t 0.0001 k3
		input+=${k1}${k2}${k3}
	elif ((flag==1));then
		#当插入一个字符时，执行的实际上是先插入字符，然后向右移动光标
		input=$'\E[C'
		remind ""
		display_content $begin_offset
		#更新屏幕并更新鼠标位置
		printf '\033[%d;%dH' $mouse_y $mouse_x
		#更新flag
		flag=0
	elif ((flag==2));then
		#更新content
		content=${content:1}
		#鼠标的移动
		mouse_x=1
		mouse_y=1
		remind ""
		display_content $begin_offset
		#更新屏幕并更新鼠标位置
		printf '\033[%d;%dH' $mouse_y $mouse_x
		#更新flag
		flag=0
		#读取下一个将要输入的内容
		read -sN1 input
		read -sN1 -t 0.0001 k1
		read -sN1 -t 0.0001 k2
		read -sN1 -t 0.0001 k3
		input+=${k1}${k2}${k3}
	elif ((flag==3));then
	#删除普通字符，执行的实际上是向左移动两次，删除一个，再向右移动一次，以适应各种不同的情况
		input=$'\E[D'
		#模拟一次左移光标
		flag=4
		remind ""
		display_content $begin_offset
		#更新屏幕并更新鼠标位置
		printf '\033[%d;%dH' $mouse_y $mouse_x
	elif ((flag==4));then
		input=$'\E[D'
		#模拟一次左移光标
		flag=5
		remind ""
		display_content $begin_offset
		#更新屏幕并更新鼠标位置
		printf '\033[%d;%dH' $mouse_y $mouse_x
	elif ((flag==5));then
	#删除并之前的标记数
		content=${content:0:$((delete_mask-1))}${content:$delete_mask}
		#默认右移
		offset=$(printf "$content" | wc -m )
		input=$'\E[C'
		remind ""
		display_content $begin_offset
		#更新屏幕并更新鼠标位置
		printf '\033[%d;%dH' $mouse_y $mouse_x
		flag=0
	fi
	done
}

#---------------------------------------
#mian函数，程序的生命哦
#---------------------------------------
function main(){
	#保证输入的参数是一个
	if [ $# -ne 1 ];then
		echo "输入的参数个数有误"
		exit 1
	fi
	#存储输入的参数
	filename="$1"
	#没有这个文件的时候，新建一个这样的文件
	if ! [ -f $filename ];then
		touch $filename
	fi
	#执行初始化函数
	Init
	#计算row和content等参数
	row=$( cat $filename | wc -l)
	content=$(cat $filename)
	#当内容为空，offset置位0
	if [[ $content = "" ]];then
		offset=0
	else
		offset=$(printf "$content" | wc -m )
	fi
	#初始化mode为0
	mode=0
	#打印提示信息
	remind=""
	#初始化begin offset为0
	begin_offset=0
	#打印文本信息
	remind "$remind"
	display_content $begin_offset
	printf '\033[%d;%dH' $mouse_y $mouse_x
	#读取输入的内容
	while read -sN1 text  
	do
	#读取输入
		read -sN1 -t 0.0001 k1
		read -sN1 -t 0.0001 k2
		read -sN1 -t 0.0001 k3
		#拼接字符串
		text+=${k1}${k2}${k3}
		case $mode in
		0) #noraml mode
			normal_deal "$text"
			#隐藏光标
			printf '\33[?25l'
			#打印文本
			remind ""
			display_content $begin_offset
			#当下一个模式是2时，特殊处理光标的位置
			if ((mode != 2));then
				printf '\033[%d;%dH' $mouse_y $mouse_x
			else
				printf '\033[%d;%dH' $height 5
			fi
			#显示光标
			printf '\33[?25h'	
			;;
		1) #insert mode 
			insert_deal $text
			#inset处理输入
			remind "$remind"
			display_content $begin_offset
			printf '\033[%d;%dH' $mouse_y $mouse_x	
			#打印文本并更新光标位置
			;;
		2) #visual mode
			visual_deal $text
			#visual处理
			remind ""
			display_content $begin_offset
			#更新光标位置，并打印文本
			printf '\033[%d;%dH' $mouse_y $mouse_x
			;;
		esac	
		
	done
	
}
#调用一次main函数
main $1