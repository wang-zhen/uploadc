# uploadc
1. ./ssdpread  sourcefile  miwenfile  devicename 
  功能：读取密文。
        把source文件的内容以密文的形式拷贝到miwenfile，miwenfile大小不小于sourcefile 且16字节对齐；
        只对方一设备有效，如果不是方一设备,则 sourcefile  内容将会和miwenfile内容一致，大小不一致  

  参数：
       sourcefile ： 源文件路径
       miwenfile  ：目的文件路径，必须存在
       devicename ：源文件对应的密文块设备路径
  举例：
      ./ssdpread  /home/fioas0/file  /root/lishi  /dev/fioa0 

2. ./ssdpwrite  miwenfile  holefile  devicename 
  功能：密文迁回。
        把miwenfile 文件的内容以密文的形式迁回到holefile，holefile文件必须存在且大小等于miwenfile ，迁回后手动设备回原始大小

  参数：
       miwenfile  ：密文文件路径，必须存在
       holefile ： 目标文件路径
       devicename ：目标文件对应块设备路径

  举例：
        比如sourcefile只有5个字节，那么miwenfile必是16个字节，holefile必须是16个字节

       #为空文件预分配磁盘空间
       dd if=/dev/zero  of=/home/fioas0/holefile  bs=16 count=1;sync

      #密文迁回
      ./ssdpwrite  /root/lishi  /home/fioas0/holefile  /dev/fioa0

      #设置回原始大小
      truncate --size 5  /home/fioas0/holefile

      #最后释放缓存
      echo 3 > /proc/sys/vm/drop_caches
      cat /home/fioas0/holefile
~                                   
