#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include "fs.h"
#include <string.h>

int main(int argc, char *argv[]) {
  int fd = open(argv[1],O_RDONLY);
  if(fd == -1){
    fprintf(stderr,"image not found.\n");
    exit(1);
  }
  
  // fstat to know the size of the file
  int rc;
  struct stat sbuf;
  rc = fstat(fd, &sbuf);
  assert(rc == 0);
  
  // Mapping the file image to the address space
  void *img = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(img != MAP_FAILED);
   
  struct superblock *sb;
  sb = (struct superblock *) (img + BSIZE);

  // Number of bitmaps blocks
  uint nbitmaps = (sb->nblocks)/(512*8) + 1;

  // Number of inode blocks
  uint nind_blocks = (sb->ninodes)/IPB + 1;
  
  // Start address of data blocks
  uint b_start =  2 + nind_blocks + nbitmaps;

  // An array to check if address has been used twice
  uint u_blocks[sb->nblocks];
  int k;
  for (k = 0; k < sb->nblocks; k++)
    u_blocks[k] = 0;
  
  // Array for storing used inodes
  uint uind_table[sb->ninodes]; // Used inodes in the table
  uint uind_dir[sb->ninodes]; // Used inodes in the direct
  for (k = 0; k < sb->nblocks; k++) {
    uind_dir[k] = 0;
    uind_table[k] = 0;
  }
  
  // Scanning through all the inodes
  int i,j;
  struct dinode *dinp = (struct dinode *) (img + 2*BSIZE);
  for (i = 0; i < sb->ninodes; i++) {

    // Error no 1 "ERROR: bad inode.\n"
    if (dinp->type != 0 && dinp->type != T_FILE && dinp->type != T_DEV && dinp->type != T_DIR) {
      fprintf(stderr,"ERROR: bad inode.\n");
      exit(1);
    }

    // Traversing the addresses pointed by all inodes
    if (dinp->type != 0) {
      for (j = 0; j < NDIRECT + 1; j++) {
        if (dinp->addrs[j] != 0) {

          // Error no 2 "ERROR: bad address in inode.\n"
          if(dinp->addrs[j] < b_start || dinp->addrs[j] >= sb->size){
            fprintf(stderr,"ERROR: bad address in inode.\n");
            exit(1);
          }

          //Error no 6: "ERROR: address used by inode but marked free in bitmap.\n"
          uint* bit_strt = (uint *)(img + (b_start - nbitmaps)*BSIZE);
          uint* byt = (uint *)(bit_strt + dinp->addrs[j]/32);
          uint offst = dinp->addrs[j] % 32;
          if (!(*byt & (1 << offst))) {
            fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
            exit(1);
          }
          
          // Error no. 8: "ERROR: address used more than once.\n"
          if (u_blocks[dinp->addrs[j] - b_start] == 1) {
            fprintf(stderr,"ERROR: address used more than once.\n");
            exit(1);
          } else {
            u_blocks[dinp->addrs[j] - b_start] = 1;
          }
        }
      }
      
      if(dinp->addrs[NDIRECT] != 0) {
        for (j = 0; j < NINDIRECT; j++) {
          uint* ndaddr = (uint*)(img + dinp->addrs[NDIRECT]*BSIZE + j*sizeof(uint));
          if (*ndaddr != 0) {

            // Error no 2 "ERROR: bad address in inode.\n"
            if(*ndaddr < b_start || *ndaddr >= sb->size){
              fprintf(stderr,"ERROR: bad address in inode.\n");
              exit(1);
            }
            
            // Error no 6: "ERROR: address used by inode but marked free in bitmap.\n"
            uint* bit_strt = (uint *)(img + (b_start - nbitmaps)*BSIZE);
            uint* byt = (uint *)(bit_strt + (*ndaddr)/32);
            uint offst = (*ndaddr) % 32;
            if (!((*byt) & (1 << offst))) {
              fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
              exit(1);
            }
            
            // Error no. 8: "ERROR: address used more than once.\n"
            if (u_blocks[(*ndaddr) - b_start] == 1) {
              fprintf(stderr,"ERROR: address used more than once.\n");
              exit(1);
            } else {
              u_blocks[(*ndaddr) - b_start] = 1;
            }
          }
        }
      }
    }
    
    // Error no 4: "ERROR: directory not properly formatted.\n"
    if (dinp->type == T_DIR) {
      struct dirent * root_dp = (struct dirent *)(img + dinp->addrs[0]*BSIZE);
      if (strncmp(root_dp->name, ".\0", 1)) {
        fprintf(stderr,"ERROR: directory not properly formatted.\n");
        exit(1);
      }
      root_dp++;
      if (strncmp(root_dp->name, "..\0", 2)) {
        fprintf(stderr,"ERROR: directory not properly formatted.\n");
        exit(1);
      }
    }

    // Error no 3: ERROR: root directory does not exist.\n"
    if (i == ROOTINO) {
      if (dinp->addrs[0] == 0) {
        fprintf(stderr,"ERROR: root directory does not exist.\n");
        exit(1);
      }
      struct dirent * root_dir = (struct dirent *)(img + dinp->addrs[0]*BSIZE);
      if (root_dir->inum != ROOTINO){
        fprintf(stderr,"ERROR: root directory does not exist.\n");
        exit(1);
      }
      root_dir++;
      if (root_dir->inum != ROOTINO){
        fprintf(stderr,"ERROR: root directory does not exist.\n");
        exit(1);
      }
    }
    
    // Error no. 9, 10
    if (dinp->type != 0)
      uind_table[i]++;
    // Traversing all the directories to identify used inodes
    if (dinp->type ==  T_DIR) {
      for (j = 0; j < NDIRECT + 1; j++) {
        if (dinp->addrs[j] != 0) {
          struct dirent * ptr = (struct dirent *)(img + dinp->addrs[j]*BSIZE);
          int n;
          for (n = 0; n < DPB; n++) {
            if(ptr->inum <= 0 && ptr->inum >= sb->ninodes){
              ptr++;
              continue;
            } 
            if ((strncmp(ptr->name, ".\0", 1) == 0) || (strncmp(ptr->name, "..\0", 2) == 0)) {
              ptr++;
              continue;
            }

            uind_dir[ptr->inum]++;
            ptr++;
          }          
        }
      }
      if(dinp->addrs[NDIRECT] != 0) {
        for (j = 0; j < NINDIRECT; j++) {
          uint* ndaddr = (uint*)(img + dinp->addrs[NDIRECT]*BSIZE + j*sizeof(uint));
          if (*ndaddr != 0) {
            struct dirent * ptr = (struct dirent *)(img + (*ndaddr)*BSIZE);
            int n;
            for (n = 0; n < DPB; n++) {
              if(ptr->inum <= 0 && ptr->inum >= sb->ninodes){
                ptr++;
                continue;
              } 
              if ((strncmp(ptr->name, ".\0", 1) == 0) || (strncmp(ptr->name, "..\0", 2) == 0)) {
                ptr++;
                continue;
              }
              uind_dir[ptr->inum]++;
              ptr++;
            }
          }
        }
      }
    }

    
    // Error no. 5 "ERROR: parent directory mismatch.\n"
    if (dinp->type ==  T_DIR && (i != 1)) {

      struct dirent * ptr = (struct dirent *)(img + dinp->addrs[0]*BSIZE);
      int cdi = ptr->inum; // Current directory inode number
      ptr++;
      int cdp = ptr->inum; // Current directory parent inode
      
      // Looking into the parent inode
      int found = 0;
      struct dinode *prnt = (struct dinode *) (img + 2*BSIZE);
      prnt += cdp;
      for (j = 0; j < NDIRECT + 1; j++) {
        if (prnt->addrs[j] != 0) {
          struct dirent *prnt_block = (struct dirent *)(img + prnt->addrs[j]*BSIZE);
          int n;
          for (n = 0; n < DPB; n++) {
            if(prnt_block->inum <= 0 && prnt_block->inum >= sb->ninodes){
              prnt_block++;
              continue;
            } 
            if ((strncmp(prnt_block->name, ".\0", 1) == 0) || (strncmp(prnt_block->name, "..\0", 2) == 0)) {
              prnt_block++;
              continue;
            }
            if (prnt_block->inum == cdi)
              found = 1;
            prnt_block++;
          }          
        }
      }
      if(prnt->addrs[NDIRECT] != 0) {
        for (j = 0; j < NINDIRECT; j++) {
        uint* ndaddr = (uint*)(img + dinp->addrs[NDIRECT]*BSIZE + j*sizeof(uint));
        if (*ndaddr != 0) {
          struct dirent *prnt_block = (struct dirent *)(img + (*ndaddr)*BSIZE);
          int n;
          for (n = 0; n < DPB; n++) {
            
            if(prnt_block->inum <= 0 && prnt_block->inum >= sb->ninodes){
              prnt_block++;
              continue;
            } 
            if ((strncmp(prnt_block->name, ".\0", 1) == 0) || (strncmp(prnt_block->name, "..\0", 2) == 0)) {
              prnt_block++;
              continue;
            }
            if (prnt_block->inum == cdi)
              found = 1;
            prnt_block++;
          }
        }
       }
      }
      
      if (found == 0) {
        fprintf(stderr,"ERROR: parent directory mismatch.\n");
        exit(1);
      }
    }
    dinp++;
    
  }
  
  
  int l;
  // Error no. 7  "ERROR: bitmap marks block in use but it is not in use.\n"
  for (l = b_start; l < (b_start + sb->nblocks); l++) {
    uint* bit_strt = (uint *)(img + (b_start - nbitmaps)*BSIZE);
    uint* byt = (uint *)(bit_strt + l/32);
    uint offst = (l % 32);
    if (*byt & (1 << offst)) {
      if (u_blocks[l-b_start] == 0){
        fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
        exit(1);
      }
    }
  }
  
  
  // Error no. 9 "ERROR: inode marked use but not found in a directory.\n""
  for(l = 2; l < sb->ninodes; l++) {
    if (uind_table[l] != 0 && uind_dir[l] == 0) {
      fprintf(stderr,"ERROR: inode marked use but not found in a directory.\n");
      exit(1);
    }
  }

  // Error no. 10 "ERROR: inode referred to in directory but marked free.\n"
  for(l = 2; l < sb->ninodes; l++) {
    if (uind_dir[l] != 0 && uind_table[l] == 0) {
      fprintf(stderr,"ERROR: inode referred to in directory but marked free.\n");
      exit(1);
    }
  }
  

  // Error no. 11
  dinp = (struct dinode *) (img + 2*BSIZE);
  for (i = 0; i < sb->ninodes; i++) {
    if (dinp->type == T_FILE) {
      if (dinp->nlink != uind_dir[i]) {
        fprintf(stderr,"ERROR: bad reference count for file.\n");
        exit(1);
      }
    }
    if (dinp->type == T_DIR && (i != 1)) {
      if (uind_dir[i] != 1) {
        fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
        exit(1);
      }
    }
    dinp++;
  }
  
  return 0;
}
