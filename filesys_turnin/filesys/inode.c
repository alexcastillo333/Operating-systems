#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define NUM_DIRECT 123     /* Number of direct data blocks for inode_disk. */
#define NUM_POINTERS 128   /* Number of pointers in an index_block. */ 
#define FIRST_LIMIT NUM_DIRECT + NUM_POINTERS 
#define MAX_DATA_SECTORS NUM_POINTERS * NUM_POINTERS + FIRST_LIMIT - 133            
  /* Maximum number of data sectors allowed by subtracting metadata sectors. */                           


/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    /* We need to add 10 direct blocks, 1 1st-level indirection, and 1 2nd-level
    indirection block. The indirection blocks are another inode_disk but 
    somehow modified. For starters, we need to figure out what information goes
    into those indirection blocks. */
    
    block_sector_t direct[NUM_DIRECT];  /* Sector indexes for direct. */
    block_sector_t singly_indirect;     /* Sector index for 1st-level. */
    block_sector_t doubly_indirect;     /* Sector index for 2nd-level. */
    bool unused[3];
    bool isdir;                        // is directory or not
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
  };



// an index_block
struct index_block {
  /* Holds 128 pointers (indexes), but we don't know what it is pointing to. 
  Does it point to index blocks or data blocks? Do we keep track of both which 
  means 256 pointers (indexes)? That would be a waste space. I think we need
  to make this more general. */
  block_sector_t blocks[NUM_POINTERS];
};  

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{

  //printf("%d\n", DIV_ROUND_UP(1, BLOCK_SECTOR_SIZE)); 1
  //printf("%d\n", DIV_ROUND_UP(2, BLOCK_SECTOR_SIZE)); 1 
  //printf("%d\n", DIV_ROUND_UP(512, BLOCK_SECTOR_SIZE)); 1
  //printf("%d\n", DIV_ROUND_UP(513, BLOCK_SECTOR_SIZE)); 2
  // div_round_up(0, block_sector_size) returns 0
  // div_round_up(1, block_sector_size) returns 1
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) {
  ASSERT (inode != NULL);
  ASSERT (pos > -1);
  struct index_block * ib = malloc(BLOCK_SECTOR_SIZE);
  block_sector_t block_index = pos / BLOCK_SECTOR_SIZE;
  if (block_index < NUM_DIRECT) {
    free(ib);
    return inode->data.direct[block_index];
  } else if (block_index < FIRST_LIMIT) {
    // read 1st level indirect block into ib
    block_read (fs_device, inode->data.singly_indirect, ib);
    block_index = ib->blocks[block_index - NUM_DIRECT];
    free(ib);
    return block_index;
  } else if (block_index < MAX_DATA_SECTORS) {  
    // maximum number of data sectors in a file is 16521
    block_index -= FIRST_LIMIT;
    // index into 2nd level index block
    block_sector_t index_into_2nd = block_index / NUM_POINTERS; 
    /* index into 2nd level index block's child, the value at this index is the 
    data_sector */
    block_sector_t index_into_2nd_child = block_index % NUM_POINTERS; 
    ASSERT (index_into_2nd < NUM_POINTERS && index_into_2nd_child 
        < NUM_POINTERS);
    // read 2nd level index block into ib
    block_read(fs_device, inode->data.doubly_indirect, ib);
    // read 2nd level index block's child into ib 
    block_read(fs_device, ib->blocks[index_into_2nd], ib);  
    block_index = ib->blocks[index_into_2nd_child];
    free(ib);
    return block_index;
  }
}


/* Return true if DATA_SECTORS + metadata sectors of free sectors exist in the 
   filesystem and update idisk's fast-file-system to point to those sectors and 
   set the data in those sectors to zeroes. Return false if there are no 
   DATA_SECTORS of free sectors in the file system */ 

                               
static bool inode_create_help(size_t data_sectors, struct inode_disk* idisk) {
  ASSERT (data_sectors >= 0 && data_sectors <= MAX_DATA_SECTORS); 
  // 16251 is largest possible file in sectors 
  // (133 metadata sectors)
  if (data_sectors == 0) {
    return true;
  }
  static char zeros[BLOCK_SECTOR_SIZE]; // Used to zero-out allocated sectors.
  struct index_block * first_ib = NULL; // Current 1st-level index_block.
  struct index_block * sec_ib = NULL;   // Current 2nd-level index_block.
  struct index_block * sec_ib_child = NULL; 
  for (int i = 0; i < data_sectors; i++) {
    if (i < NUM_DIRECT) {
      if (free_map_allocate(1, &idisk->direct[i])) {
        block_write (fs_device, idisk->direct[i], zeros);
      } else {
        return false;
      }
    }
    else if (i == NUM_DIRECT) { // direct index blocks not enough to store file
      // i is 123
      free_map_allocate(1, &idisk->singly_indirect); // allocate 1st level ib
      first_ib = malloc(sizeof(struct index_block));
      if (free_map_allocate(1, &first_ib->blocks[0])) {
        block_write (fs_device, first_ib->blocks[0], zeros);
      } else {
        free(first_ib);
        return false;
      }
    }
    else if (i < FIRST_LIMIT) {
      //  124 <= i < 251
      if (free_map_allocate(1, &first_ib->blocks[i - NUM_DIRECT])) {
        block_write(fs_device, first_ib->blocks[i - NUM_DIRECT], zeros);
      } else {
        free(first_ib);
        return false;
      }
    }
    else if (i == FIRST_LIMIT) { // 1st level ib not enough to store file
      // i is 251
      free_map_allocate(1, &idisk->doubly_indirect); // allocate 2nd level IB
      sec_ib = malloc(sizeof(struct index_block));
      free_map_allocate(1, &sec_ib->blocks[0]);
      sec_ib_child = malloc(sizeof(struct index_block));
      if (free_map_allocate(1, &sec_ib_child->blocks[0])) {
        block_write(fs_device, sec_ib_child->blocks[0], zeros);
      } else {
        free(sec_ib_child);
        free(sec_ib);
        free(first_ib);
        return false;
      }
    }
    else if (i < MAX_DATA_SECTORS) {
      // 252 <= i < 16502
      if ((i - FIRST_LIMIT) % NUM_POINTERS == 0) {
        /* need to write previous sec_ib_child to disk and allocate new 
           sec_ib_child */
        /* We need to make a new 2nd-level IB child */ 
        block_write(fs_device, 
            sec_ib->blocks[(i - FIRST_LIMIT) / NUM_POINTERS] , sec_ib_child);
        free(sec_ib_child);            
        sec_ib_child = malloc(sizeof(struct index_block));
        if (free_map_allocate(1, &sec_ib_child->blocks[0])) {
          block_write(fs_device, sec_ib_child->blocks[0], zeros);
        } else {
          free(sec_ib_child);
          free(sec_ib);
          free(first_ib);
          return false;
        }
      } else {
        if (free_map_allocate(1, 
              &sec_ib_child->blocks[(i - FIRST_LIMIT) % NUM_POINTERS])) {    
          // this allocates a data sector pointed to by sec_ib_child
          block_write(fs_device, 
              sec_ib_child->blocks[(i - FIRST_LIMIT) % NUM_POINTERS], zeros);
        } else {
          free(sec_ib_child);
          free(sec_ib);
          free(first_ib);
          return false;
        }
      }
      /* If the current data sector we are allocating is the last one we need
      to allocate, we need to write the sec_ib_child to sec_ib */
      if (i == data_sectors - 1) {
        block_write(fs_device, sec_ib->blocks[(i - FIRST_LIMIT) / NUM_POINTERS],
            sec_ib_child);
      }
    } else {
      /* At this point, we have gone over maximum number of allowed data sectors
       for a file. */
      return false;
    }
  }
  // write the index blocks to disk, return true
  if (first_ib != NULL) {
    block_write(fs_device, idisk->singly_indirect, first_ib);
    free(first_ib);
  }
  if (sec_ib != NULL) {
    block_write(fs_device, idisk->singly_indirect, sec_ib);
    free(sec_ib);
  }
  return true;
}

// Deallocate NUM_SECTORS of data sectors and the metadata sectors of idisk
static void close_help(struct inode_disk * idisk, int num_sectors) {
  struct index_block* current_2nd = NULL; // Current 1st-level index block.
  struct index_block* current_1st = NULL; // Current 2nd-level index block.
  for (int i = 0; i < num_sectors; i++) {
    if (i < NUM_DIRECT) {
      free_map_release(idisk->direct[i], 1);
    } else if (i == NUM_DIRECT) {
      /* We will release the 1st-level index block's first data block. The 
      current_1st now points to the 1st-level index block. */
      current_1st = malloc(sizeof(struct index_block));
      block_read(fs_device, idisk->singly_indirect, current_1st);
      free_map_release(current_1st->blocks[0], 1);
    } else if (i < FIRST_LIMIT) {
      free_map_release(current_1st->blocks[i - NUM_DIRECT], 1);
    } else if (i == FIRST_LIMIT) {
      /* We will release the first data block in the 2nd-level index block. At 
      this point the current_1st is freed and points to the child of the 
      2nd-level. The current_2nd now points to the 2nd-level. */
      current_2nd = malloc(sizeof(struct index_block));
      free(current_1st);
      current_1st = malloc(sizeof(struct index_block));
      block_read(fs_device, idisk->doubly_indirect, current_2nd);
      block_read(fs_device, current_2nd->blocks[0], current_1st);
      free_map_release(current_1st->blocks[0], 1);
    } else if (i < MAX_DATA_SECTORS) {
      if ((i - FIRST_LIMIT) % NUM_POINTERS == 0) {
        // At this point the current 1st-level is the child of the 2nd-level
        // We want to release the previous child
        free_map_release(current_2nd->blocks[(i - FIRST_LIMIT) / 
            NUM_POINTERS - 1], 1);
        free(current_1st);
        current_1st = malloc(sizeof(struct index_block));
        // Set the current 1st-level to the next child
        block_read(fs_device, current_2nd->blocks[(i - FIRST_LIMIT) /
            NUM_POINTERS], current_1st);
      } else {
        // Free the leaves
        free_map_release(current_1st->blocks[(i - FIRST_LIMIT) % 
            NUM_POINTERS], 1);
        if (i == num_sectors - 1) {
          /* If this is the last sector and we are still inside a 1st_level, 
          release the one we are in. */
          free_map_release(current_2nd->blocks[(i - FIRST_LIMIT) /
              NUM_POINTERS], 1);
        }
      }
    }
  }
  if (current_1st != NULL) {
    // Assumes that CURRENT_1ST was malloc-ed and i was at least 123
    free_map_release(idisk->singly_indirect, 1);
    free(current_1st);
  }
  if (current_2nd != NULL) {
    // Assumes that CURRENT_2ND was malloc-ed and i was at least 251
    free_map_release(idisk->doubly_indirect, 1);
    free(current_2nd);
  }
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}


/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool isdir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;
  ASSERT (length >= 0);  
  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      // Length is what we want the file to be. Not what it is now.
      // Do we assign disk_inode->length before or after extend_file()?
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->isdir = isdir;
      success = inode_create_help(sectors, disk_inode);
      if (success) {
        block_write(fs_device, sector, disk_inode);
      }
      free (disk_inode);
    }
  return success;
}


/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk. (Does it?  Check code.)
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          close_help(&inode->data, bytes_to_sectors(inode->data.length)); 
        }
      free (inode); 
    }
}


/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}


// extend file of idisk by size_to_extend bytes. return true if successful
bool
extend_file (struct inode_disk *idisk, int size_to_extend) {
  // starting index into the file array of sectors of the extension
  int start = bytes_to_sectors(idisk->length);
  int end = start + bytes_to_sectors(size_to_extend);
  static char zeros[BLOCK_SECTOR_SIZE]; // Used to zero-out allocated sectors.
  struct index_block * first_ib = NULL; // Current 1st-level index_block.
  struct index_block * sec_ib = NULL;   // Current 2nd-level index_block.
  struct index_block * sec_ib_child = NULL; 
  for (int i = start; i < end; i++) {
    if (i < NUM_DIRECT) {
      if (free_map_allocate(1, &idisk->direct[i])) {
        block_write (fs_device, idisk->direct[i], zeros);
      } else {
        return false;
      }
    }
    else if (i == NUM_DIRECT) { // direct index blocks not enough to store file
      // i is 123
      free_map_allocate(1, &idisk->singly_indirect); // allocate 1st level ib
      first_ib = malloc(sizeof(struct index_block));
      if (free_map_allocate(1, &first_ib->blocks[0])) {
        block_write (fs_device, first_ib->blocks[0], zeros);
      } else {
        free(first_ib);
        return false;
      }
    }
    else if (i < FIRST_LIMIT) {
      //  124 <= i < 251
      if (free_map_allocate(1, &first_ib->blocks[i - NUM_DIRECT])) {
        block_write(fs_device, first_ib->blocks[i - NUM_DIRECT], zeros);
      } else {
        free(first_ib);
        return false;
      }
    }
    else if (i == FIRST_LIMIT) { // 1st level ib not enough to store file
      // i is 251
      free_map_allocate(1, &idisk->doubly_indirect); // allocate 2nd level IB
      sec_ib = malloc(sizeof(struct index_block));
      free_map_allocate(1, &sec_ib->blocks[0]);
      sec_ib_child = malloc(sizeof(struct index_block));
      if (free_map_allocate(1, &sec_ib_child->blocks[0])) {
        block_write(fs_device, sec_ib_child->blocks[0], zeros);
      } else {
        free(sec_ib_child);
        free(sec_ib);
        free(first_ib);
        return false;
      }
    }
    else if (i < MAX_DATA_SECTORS) {
      // 252 <= i < 16502
      if ((i - FIRST_LIMIT) % NUM_POINTERS == 0) {
        /* need to write previous sec_ib_child to disk and allocate new 
           sec_ib_child */
        /* We need to make a new 2nd-level IB child */ 
        block_write(fs_device, 
            sec_ib->blocks[(i - FIRST_LIMIT) / NUM_POINTERS] , sec_ib_child);
        free(sec_ib_child);            
        sec_ib_child = malloc(sizeof(struct index_block));
        if (free_map_allocate(1, &sec_ib_child->blocks[0])) {
          block_write(fs_device, sec_ib_child->blocks[0], zeros);
        } else {
          free(sec_ib_child);
          free(sec_ib);
          free(first_ib);
          return false;
        }
      } else {
        if (free_map_allocate(1, 
              &sec_ib_child->blocks[(i - FIRST_LIMIT) % NUM_POINTERS])) {    
          // this allocates a data sector pointed to by sec_ib_child
          block_write(fs_device, 
              sec_ib_child->blocks[(i - FIRST_LIMIT) % NUM_POINTERS], zeros);
        } else {
          free(sec_ib_child);
          free(sec_ib);
          free(first_ib);
          return false;
        }
      }
      /* If the current data sector we are allocating is the last one we need
      to allocate, we need to write the sec_ib_child to sec_ib */
      if (i == end - 1) {
        block_write(fs_device, sec_ib->blocks[(i - FIRST_LIMIT) / NUM_POINTERS],
            sec_ib_child);
      }
    } else {
      /* At this point, we have gone over maximum number of allowed data sectors
       for a file. */
      return false;
    }
  }
  // write the index blocks to disk, return true
  if (first_ib != NULL) {
    block_write(fs_device, idisk->singly_indirect, first_ib);
    free(first_ib);
  }
  if (sec_ib != NULL) {
    block_write(fs_device, idisk->singly_indirect, sec_ib);
    free(sec_ib);
  }
  idisk->length += size_to_extend;
  return true;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  /* check that we are not trying to write in a file at position 
    that is bigger than our filesystem supports */
  ASSERT((size + offset) / BLOCK_SECTOR_SIZE < MAX_DATA_SECTORS);

  // first check if inode->inode_disk.length < offset + size, extend file in 
  //that case
  if (inode->data.length < offset + size) {
    extend_file(&inode->data, offset + size - inode->data.length);
    block_write(fs_device, inode->sector, &inode->data);
  }


  if (inode->deny_write_cnt)
    return 0;

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

bool inode_isdir (struct inode *inode) {
  return inode->data.isdir;
}
