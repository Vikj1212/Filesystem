#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports four arguments

#define BLOCK_SIZE 1024
#define NUM_BLOCKS 65536
#define BLOCKS_PER_FILE 1024
#define NUM_FILES 256
#define FIRST_DATA_BLOCK 1001
#define MAX_FILE_SIZE 1048576

uint8_t data[NUM_BLOCKS][BLOCK_SIZE];

//512 blocks for free block map
uint8_t *free_blocks;

uint8_t *free_inodes;

struct _directoryEntry
{
    char filename[64];
    short in_use;
    int32_t inode;
};

struct _directoryEntry * directory;

struct inode
{
    int32_t block[BLOCKS_PER_FILE];
    short in_use;
    uint8_t attribute;
    uint32_t file_size;
    char time[64]; 
};

struct inode *inodes;

FILE * fp;

char image_name[64];
int image_open;


int32_t findFreeBlock()
{
  for(int i = 0; i < NUM_BLOCKS; i++)
  {
    if(free_blocks[i+1001])
    {
      return i + 1001;
    }
  }
  return -1;
}
int32_t findFreeInode()
{
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(free_inodes[i])
    {
      return i;
    }
  }
  return -1;
}
int32_t findFreeInodeBlock(int32_t inode)
{
  for(int i = 0; i < BLOCKS_PER_FILE; i++)
  {
    if(inodes[inode].block[i] == -1)
    {
      return i;
    }
  }
  return -1;
}
void init()
{
  directory = (struct _directoryEntry *)&data[0][0];
  inodes = (struct inode *)&data[20][0];
  free_blocks = ( uint8_t *)&data[1000][0];
  free_inodes = (uint8_t *)&data[19][0];

  memset(image_name, 0, 64);
  image_open = 0;

  for(int i = 0; i < NUM_FILES; i++)
  {
    directory[i].in_use = 0;
    directory[i].inode = -1;
    free_inodes[i] = 1;
    memset(directory[i].filename, 0, 64);

    for(int j = 0; j < NUM_BLOCKS; j++)
    {
      inodes[i].block[j] = -1;
      inodes[i].in_use = 0;
      inodes[i].attribute = 0;
      inodes[i].file_size = 0;
      memset(inodes[i].time, 0, 64);
    }
  }
  for(int j = 0; j < NUM_BLOCKS; j++)
  {
    free_blocks[j] = 1;
  }
}

void attrib_file(char *filename, char *attr)
{
  int32_t d_index = -1;
  for(int32_t i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(directory[i].filename, filename) == 0)
    {
      d_index = i;
    }
  }
  if(d_index == -1)
  {
    printf("ERROR: File not found in directory.\n");
    return;
  }
  if(strcmp("+h", attr) == 0)
  {
    if( inodes[directory[d_index].inode].attribute == 2 || inodes[directory[d_index].inode].attribute == 3 )
    {
      inodes[directory[d_index].inode].attribute = 3;
    }else
    {
      inodes[directory[d_index].inode].attribute = 1;
    }
    return;
  }
  if(strcmp("-h", attr) == 0)
  {
    if( inodes[directory[d_index].inode].attribute == 3 || inodes[directory[d_index].inode].attribute == 2 )
    {
      inodes[directory[d_index].inode].attribute = 2;
    }else
    {
      inodes[directory[d_index].inode].attribute = 0;
    }
    return;
  }
  if(strcmp("+r", attr) == 0)
  {
    if( inodes[directory[d_index].inode].attribute == 1 || inodes[directory[d_index].inode].attribute == 3 )
    {
      inodes[directory[d_index].inode].attribute = 3;
    }else
    {
      inodes[directory[d_index].inode].attribute = 2;
    }
    return;
  }
  if(strcmp("-r", attr) == 0)
  {
    if( inodes[directory[d_index].inode].attribute == 3 || inodes[directory[d_index].inode].attribute == 2 )
    {
      inodes[directory[d_index].inode].attribute = 1;
    }else
    {
      inodes[directory[d_index].inode].attribute = 0;
    }
    return;
  }
  printf("No appropriate attr symbol given.\n attr: +/-h or +/-r\n");
}

int dfs()
{
  int count = 0;
  for(int j = FIRST_DATA_BLOCK; j < NUM_BLOCKS; j++)
  {
    if(free_blocks[j])
    {
      count++;
    }
  }
  return count*BLOCK_SIZE;
}
void createfs(char * filename)
{
  fp = fopen(filename, "w");
  strncpy(image_name, filename, strlen(filename));
  memset(data, 0, NUM_BLOCKS*BLOCK_SIZE);
  image_open = 1;
  
  directory = (struct _directoryEntry *)&data[0][0];
  inodes = (struct inode *)&data[20][0];
  free_blocks = ( uint8_t *)&data[1000][0];
  free_inodes = (uint8_t *)&data[19][0];

  for(int i = 0; i < NUM_FILES; i++)
  {
    directory[i].in_use = 0;
    directory[i].inode = -1;
    free_inodes[i] = 1;
    memset(directory[i].filename, 0, 64);

    for(int j = 0; j < NUM_BLOCKS; j++)
    {
      inodes[i].block[j] = 0;
      inodes[i].in_use = 0;
      inodes[i].attribute = 0;
      inodes[i].file_size = 0;
      memset(inodes[i].time, 0, 64);
    }
  }
  for(int j = 0; j < NUM_BLOCKS; j++)
  {
    free_blocks[j] = 1;
  }

  fclose(fp);
}
void savefs()
{
  fp = fopen(image_name, "w");
  fwrite(&data[0][0], BLOCK_SIZE, NUM_BLOCKS, fp);
  memset(image_name, 0, 64);
  image_open = 0;
  fclose(fp);
}
void openfs(char * filename)
{
  fp = fopen(filename, "r");
  strncpy(image_name, filename, strlen(filename));
  fread(&data[0][0], BLOCK_SIZE, NUM_BLOCKS, fp);
  image_open = 1;
  //fclose(fp);
}
void closefs()
{
  fclose(fp);
  image_open = 0;
  memset(image_name, 0, 64);
}
void list()
{
  int not_found = 1;

  for(int i = 0; i < NUM_FILES; i++)
  {
    if((directory[i].in_use) && (inodes[directory[i].inode].attribute == 0 || inodes[directory[i].inode].attribute == 2))
    {
      not_found = 0;
      char filename[65];
      memset(filename, 0, 65);
      strncpy(filename, directory[i].filename, strlen(directory[i].filename));
      printf("%s %d Bytes %s\n", filename, inodes[directory[i].inode].file_size, inodes[directory[i].inode].time);
    }
  }

  if(not_found)
  {
    printf("Error: No Files Found!\n");
  }
}

void insert(char *filename)
{
  //if filename is NULL
  if(filename == NULL)
  {
    printf("ERROR: Filename is NULL\n");
    return;
  }
  //verify that the file exists
  struct stat buf;
  int ret = stat(filename, &buf);
  
  if(ret == -1)
  {
    printf("ERROR: File does not exist\n");
    return;
  }

  //verify that the file isnt to big
  if(buf.st_size > MAX_FILE_SIZE)
  {
    printf("ERROR: File Size exceeds capacity\n");
    return;
  }
  
  if(buf.st_size > dfs())
  {
    printf("ERROR: Not enough free memory for file size\n");
    return;
  }

  //find an empty directory entry
  int directory_found = -1;
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(directory[i].in_use == 0)
    {
      directory_found = i;
      break;
    }
  }
  if(directory_found == -1)
  {
    printf("ERROR: No Empty directory entry found!\n");
    return;
  }

// Open the input file read-only 
  FILE *ifp = fopen ( filename, "r" ); 
  printf("Reading %d bytes from %s\n", (int) buf . st_size, filename );
 
    // Save off the size of the input file since we'll use it in a couple of places and 
    // also initialize our index variables to zero. 
  int32_t copy_size   = buf . st_size;

    // We want to copy and write in chunks of BLOCK_SIZE. So to do this 
    // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
    // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
  int32_t offset      = 0;               

    // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big 
    // memory pool. Why? We are simulating the way the file system stores file data in
    // blocks of space on the disk. block_index will keep us pointing to the area of
    // the area that we will read from or write to.
  int32_t block_index = -1;

  //Find free inode
  int32_t inode_index = findFreeInode();

  //place file info into directory
  directory[directory_found].in_use = 1;
  directory[directory_found].inode = inode_index;
  strncpy(directory[directory_found].filename, filename, strlen(filename));

  inodes[inode_index].file_size = buf.st_size;
 
  // copy_size is initialized to the size of the input file so each loop iteration we
  // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
  // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
  // we have copied all the data from the input file.
  while( copy_size > 0 )
  {
    // Index into the input file by offset number of bytes.  Initially offset is set to
    // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We 
    // then increase the offset by BLOCK_SIZE and continue the process.  This will
    // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
    fseek( ifp, offset, SEEK_SET );

    block_index = findFreeBlock();
    if(block_index == -1)
    {
      printf("ERROR: No free blocks found!\n");
      return;
    }

    // Read BLOCK_SIZE number of bytes from the input file and store them in our
    // data array. 
    int32_t bytes  = fread( data[block_index], BLOCK_SIZE, 1, ifp );

    //save the block in inode
    int32_t inode_block = findFreeInodeBlock(inode_index);
    inodes[inode_index].block[inode_block] = block_index;

      // If bytes == 0 and we haven't reached the end of the file then something is 
      // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
      // It means we've reached the end of our input file.
    if( bytes == 0 && !feof( ifp ) )
    {
      printf("ERROR: An error occured reading from the input file.\n");
      return;
    }

    // Clear the EOF file flag.
    clearerr( ifp );

    // Reduce copy_size by the BLOCK_SIZE bytes.
    copy_size -= BLOCK_SIZE;
      
    // Increase the offset into our input file by BLOCK_SIZE.  This will allow
    // the fseek at the top of the loop to position us to the correct spot.
    offset    += BLOCK_SIZE;

    free_blocks[block_index] = 0;

      // Increment the index into the block array 
      // DO NOT just increment block index in your file system
    block_index = findFreeBlock();
  }

  time_t period = time(NULL);
  struct tm *dTM = localtime(&period);
  char s_time[64];
  size_t t_check = strftime(s_time, sizeof(s_time), "%c", dTM);
  strncpy(inodes[inode_index].time, s_time, strlen(s_time));
  if(t_check > 0)
  {
    //Just for unused error;
  }
  // We are done copying from the input file so close it out.
  fclose( ifp );
  

}
void deleteFile(char *filename)
{
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(directory[i].filename, filename)==0 && (inodes[directory[i].inode].attribute == 0 || inodes[directory[i].inode].attribute == 1))
    {
      directory[i].in_use = 0;
      return;
    }
  }
  printf("No such file found or file is read-only.\n");
}
void undeleteFile(char *filename)
{
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(directory[i].filename, filename)==0)
    {
      directory[i].in_use = 1;
      return;
    }
  }
  printf("Unable to undelete file.\n");
}
void retrieve(char *filename, char *new_filename)
{
  int index = -1;
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(directory[i].filename, filename) == 0)
    {
      index = i;
      break;
    }
  }
  if(index == -1)
  {
    printf("ERROR: File not found.\n");
    return;
  }
  char *fp_name;
  if(new_filename != NULL)
  {
    fp_name = malloc(strlen(new_filename)*sizeof(char)+1);
    strcpy(fp_name, new_filename);
  }else
  {
    fp_name = malloc(strlen(filename)*sizeof(char)+1);
    strcpy(fp_name, filename);
  }
  //char newFile[100] = strcat("/workspaces/Filesystem/");
  //FILE ifp;
  //ifp = fopen(filename, "r");
  int32_t NF_size = inodes[directory[index].inode].file_size;
  FILE *ofp;
  ofp = fopen(fp_name, "w");
  int i = 0;
  while(NF_size > 0)
  {
    uint8_t info[1024];
    //int32_t i_byte = fread( info, BLOCK_SIZE, 1, inodes[directory[index].inode].block[i] );
    if( NF_size >= 1024 )
    {
      int32_t o_byte = fwrite( &data[inodes[directory[index].inode].block[i]], BLOCK_SIZE, 1, ofp );
    }
    else
    {
      int32_t o_byte = fwrite( &data[inodes[directory[index].inode].block[i]], NF_size, 1, ofp );
    }
    //fprintf( ofp, "%s", inodes[directory[index].inode].block[i] );
    NF_size = NF_size - BLOCK_SIZE;
    i++;
  }

  free(fp_name);
  fclose(ofp);
}




int main()
{
  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  init();
  image_open = 0;
  fp = NULL;

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup( command_string );                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    if(strcmp("createfs", token[0]) == 0)
    {
      if(token[1] == NULL)
      {
        printf("ERROR: No filename specified!\n");
        continue;
      }
      createfs(token[1]);
    }
     
    if(strcmp("savefs", token[0]) == 0)
    {
      if(image_open == 0)
      {
        printf("ERROR: No file system created!\n");
        continue;
      }
      savefs();
    }

    if(strcmp("open", token[0]) == 0)
    {
      if(token[1] == NULL)
      {
        printf("ERROR: No filename specified!\n");
        continue;
      }
      openfs(token[1]);
    }
    if(strcmp("close", token[0]) == 0)
    {
      if(image_open == 0)
      {
        printf("ERROR: No file system open!\n");
        continue;
      }
      closefs();
    }
    if(strcmp("list", token[0]) == 0)
    {
      if(image_open == 0)
      {
        printf("ERROR: No Disk Image open!");
        continue;
      }
      list();
    }
    if(strcmp("df", token[0]) == 0)
    {
      if(image_open == 0)
      {
        printf("ERROR: No Disk Image open!");
        continue;
      }
      printf("%d Bytes Free\n", dfs());
    }
    if(strcmp("insert", token[0]) == 0)
    {
      if(image_open == 0)
      {
        printf("ERROR: No disk image is open\n");
        continue;
      }
      if(token[1] == NULL)
      {
        printf("ERROR: No filename given\n");
        continue;
      }
      insert(token[1]);
    }
    if(strcmp("attrib", token[0])==0)
    {
      if (token[1] == NULL || token[2] == NULL)
      {
        printf("ERROR: Attribute or Filename not specified.\n");
        continue;
      }
      attrib_file(token[1], token[2]);
    }
    if(strcmp("delete", token[0])==0)
    {
      if(token[1] == NULL)
      {
        printf("ERROR: Filename not specified.\n");
        continue;
      }
      deleteFile(token[1]);
    }
    if(strcmp("retrieve", token[0]) == 0)
    {
        if(token[1] == NULL)
        {
            printf("ERROR: No file specified.\n");
            continue;
        }
        retrieve(token[1], token[2]);
    }
    if(strcmp("quit", token[0]) == 0)
    {
      for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
     {
        if( token[i] != NULL )
        {
          free( token[i] );
        }
      }

      free( head_ptr );
      break;

    }
     
     
     
     // Cleanup allocated memory
    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      if( token[i] != NULL )
      {
        free( token[i] );
      }
    }

    free( head_ptr );

  }

  free( command_string );

}