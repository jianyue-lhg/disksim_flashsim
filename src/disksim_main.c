/*
 * DiskSim Storage Subsystem Simulation Environment (Version 3.0)
 * Revision Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001, 2002, 2003.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty" statements
 * are included with all reproductions and derivative works and associated
 * documentation. This software may also be redistributed without charge
 * provided that the copyright and "No Warranty" statements are included
 * in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 * COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE
 * OR DOCUMENTATION.
 *
 */



/*
 * DiskSim Storage Subsystem Simulation Environment (Version 2.0)
 * Revision Authors: Greg Ganger
 * Contributors: Ross Cohen, John Griffin, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 1999.
 *
 * Permission to reproduce, use, and prepare derivative works of
 * this software for internal use is granted provided the copyright
 * and "No Warranty" statements are included with all reproductions
 * and derivative works. This software may also be redistributed
 * without charge provided that the copyright and "No Warranty"
 * statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 */


#include "disksim_global.h"
//flshsim
#include "ssd_interface.h"
#include "rbtree.h"
#define WARMFLASH_DEDUP 1
#define DEBUG_RBTREE    1


extern RBRoot *rev_root;
extern RBRoot *dedup_root;
extern RBRoot *cache_root;	

extern int max_laddr;




void warmFlashsynth(){

  memset(dm_table, -1, sizeof(int) * DM_MGR_SIZE);

  nand_stat_reset();
  reset_flash_stat();

  if(ftl_type == 3){
    opagemap_reset();
  }

  else if(ftl_type == 4)
  {
    write_count = 0;
    read_count = 0;
  }
}

void warmFlash(char *tname){
  struct data_node *data_tmp=NULL;
   Node *tree_node_tmp=NULL;
  
  FILE *fp = fopen(tname, "r");
  char buffer[80];
  double time;
  int devno, bcount, flags=0;
  long int blkno;
  double delay;
  int i;
  char hash[33];
  int dele_paddr;
  Node *dele_tree_node=NULL;
  int start_sect;
  //struct data_node *cover_data_tmp=NULL;
  while(fgets(buffer, sizeof(buffer), fp)){
  	if(strlen(buffer)<30)
		continue;
    sscanf(buffer, "%lf %d %d %d %s\n", &time, &blkno, &bcount, &flags,hash);
	printf("disksim_main.c 92 %lf %d %d %d %s\n",time, blkno, bcount, flags,hash);
    devno = 0;
    //bcount = ((blkno + bcount -1) / 4 - (blkno)/4 + 1) * 4;
	bcount =4;
    blkno /= 4;
    blkno *= 4;
    blkno %= (max_laddr-100);
    blkno+=100;
   //-----------------
  
   #if WARMFLASH_DEDUP
    tree_node_tmp=iterative_rbtree_search_hash(dedup_root, hash);
    if(tree_node_tmp==NULL){
    	data_tmp = create_data_node(-1,blkno, 1, hash);
		insert_rbtree_laddr(cache_root, data_tmp);
	}else{
		printf("tree_node_tmp!=NULL\n");
		if(!search_laddr_in_node(tree_node_tmp->key, blkno)){
			//为什么这儿加入覆盖检测会有问题，因为pagemap是在后面的代码中才配置的，此时是读不到pagemap的
 			/*//------------------覆盖检测与处理-----------------------------
			if(pagemap[blkno].free == 0){
				printf("main.c 134\n");
                dele_paddr=pagemap[blkno].ppn;
				printf("delete_paddr is %d\n");
				dele_tree_node=iterative_rbtree_search_paddr(rev_root, dele_paddr);
			    if(dele_tree_node){
					dele_tree_node->key->ref--;
					if(dele_tree_node->key->ref<=0){
						delete_data_node(dele_tree_node->key, dedup_root,  rev_root);
					}
					start_sect = pagemap[blkno].ppn * SECT_NUM_PER_PAGE;
   			    	for(i = 0; i<SECT_NUM_PER_PAGE; i++){
      					nand_invalidate(start_sect + i, blkno*SECT_NUM_PER_PAGE + i);
    				} 
    				nand_stat(3);
				}			
			}
			//------------------覆盖检测与处理-----------xia---------------*/
     	    tree_node_tmp->key->ref++;
		    insert_laddr_in_node(tree_node_tmp->key,blkno);
	    }
		continue;
	}
	#endif
   //-----------------
	
    delay = callFsim(blkno, bcount, 0);   

    for(i = blkno; i<(blkno+bcount); i++){ dm_table[i] = DEV_FLASH; }
  }
  nand_stat_reset();

  if(ftl_type == 3) opagemap_reset(); 

  else if(ftl_type == 4) {
    write_count = 0; read_count = 0; }

  fclose(fp);
  printf("warmupFlash run over\n");
}
	/*void warmFlash(char *tname)  //----------需要在这儿加入iotrace_read中的代码包括建树等
		{
	
	  FILE *fp = fopen(tname, "r");
	  char buffer[80];
	  double time;
	  int devno, bcount, flags;
	  long int blkno;
	  double delay;
	  int i;
	  char rw;
	  char hash[33];
	  int dele_paddr;
	  while(fgets(buffer, sizeof(buffer), fp)){
		//sscanf(buffer, "%lf %d %d %d %d\n", &time, &devno, &blkno, &bcount, &flags); //--lhg
		if(strlen(buffer)<33)
			continue;
		sscanf(buffer, "%lf %d %d %d %s \n", &time,  &blkno,&bcount,&flags,hash);
		printf("warmFlash blkno 1 is %d  flags is %x\n",blkno,flags);
	   bcount =4;
	   blkno /= 4;	//----修改逻辑地址范围
	   blkno *= 4;
	   blkno %= (max_laddr-100);
       blkno+=100;
	   devno=0;
		printf("warmFlash blkno 2 is %d\n",blkno);
		delay = callFsim(blkno, bcount, 0);   
		printf("warmFlash blkno 3 is %d\n",blkno);
		for(i = blkno; i<(blkno+bcount); i++){ dm_table[i] = DEV_FLASH; }
	  }
	  nand_stat_reset();
	
	  if(ftl_type == 3) opagemap_reset(); 
	
	  else if(ftl_type == 4) {
		write_count = 0; read_count = 0; }
	   printf("warmFlash blkno 4 is %d\n",blkno);
	  fclose(fp);
	  printf("************************************out warmFlash\n");
	}*/
	
	int read_size(char *str)
	{
		int i=0,j,k=0;
		int flag=0;
		char line[500];
		char sz[20];
		int num=-1;
		FILE *fp;
		fp=fopen(str,"r");
		while(!feof(fp))
		{
			memset(line, '\0', 500);
			fgets(line,500,fp);
			i++;
			if(i==124)
				break;
		}
		//printf("%s\n",line);
		for(j=0;j<strlen(line);j++)
		{
			if(line[j]=='=')
			{
				flag=1;
				continue;
			}
			if(flag == 1&&line[j]!=' ')
			{
				sz[k++]=line[j];
				continue;
			}
	
		}
		num=atoi(sz);
		fclose(fp);
		
		return num;
	}



int main (int argc, char **argv)
{
  int i;
  int len;
  void *addr;
  void *newaddr;
  int j;
  rev_root=create_rbtree();
  dedup_root=create_rbtree();
  cache_root=create_rbtree();
  max_laddr = read_size(argv[1]);
  printf("max_laddr is %d\n",max_laddr); 	
  if(argc == 2) {
     disksim_restore_from_checkpoint (argv[1]);
  } 
  else {
    len = 8192000 + 2048000 + ALLOCSIZE;
    addr = malloc (len);
    newaddr = (void *) (rounduptomult ((long)addr, ALLOCSIZE));
    len -= ALLOCSIZE;

    disksim = disksim_initialize_disksim_structure (newaddr, len);
    disksim_setup_disksim (argc, argv);
  }

  memset(dm_table, -1, sizeof(int)*DM_MGR_SIZE);

  if(ftl_type != -1){

    initFlash();
    reset_flash_stat();
    nand_stat_reset();
  }

 // warmFlashsynth();
  //warmFlash(argv[4]);
  
  disksim_run_simulation ();
  #if DEBUG_RBTREE
  inorder_rbtree(dedup_root);
  printf("------------------------\n");
  inorder_rbtree(rev_root);
  #endif
  disksim_cleanup_and_printstats ();
  if(dedup_root->node == NULL)
  	printf("no dedup_root\n");
  //printf("%d %d %d %s\n", dedup_root->node->key->paddr,dedup_root->node->key->laddr_l->laddr,dedup_root->node->key->ref,dedup_root->node->key->md);
  //inorder_rbtree(cache_root);
  printf("has run over\n");

  return 0;
}
