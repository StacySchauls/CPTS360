   #! /bin/bash
  sudo dd if=/dev/zero of=mydisk bs=1024 count=1440
  sudo  mkfs -b 1024 mydisk 1440
  sudo  mount -o loop mydisk /mnt
   (cd /mnt; mkdir dir1 dir2 dir3 dir4; touch file1 file2 file3 file4; ls -l)
   sudo umount /mnt
