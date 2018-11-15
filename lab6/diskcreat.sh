  #! /bin/bash
   dd if=/dev/zero of=mydisk bs=1024 count=1440
   mkfs -b 1024 mydisk 1440
   mount -o loop mydisk /mnt
   (cd /mnt; mkdir dir1 dir2 dir3 dir4; mkdir dir1/dir5 dir2/dir7; touch file1 file2 file3 file4 dir1/dir5/file5 dir2/dir7/file6; ls -l)
   umount /mnt
