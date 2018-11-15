#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

typedef struct node
{
	char name[64];		// Name of the node
	char type;		// Type of node, either 'D' for directory or 'F' for file
	struct node *childPtr,*siblingPtr,*parentPtr;
}NODE;

//globals

NODE *root, *cwd;		// Pointers to the root NODE and to the "Current Working Directory" NODE
char line[128];			// User Input Line
char command[16], pathname[64];	// User inputs
char dname[64], bname[64];	// String holders
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm", "quit", "menu", "reload", "save", 0};	// List of commands accepted by this simulation

//prototypes

void mkrdir(void);
void rmdir(void);
void ls(void);
void cd(void);
void pwd(void);
void creat(void);
void rm(void);
void quit(void);
void menu(void);
void reload(void);
void save(void);
void rsave(FILE *outfile, NODE *node);
void removeNewLine(char *string);
void printHeader(void);
void rpwd(NODE *node);
void splitPath(char *path);
void dbname(char *path);
int hasSpace(void);
int hasPath(void);
NODE *search_child(char *dname, NODE *child);
NODE *findDir(NODE *root);
void insertNode(NODE *parent, NODE *newNode);
void displayPath(NODE *node);
void memReset(void);
void displayNode(NODE *node);
void deleteNode(NODE *node);
void rPrintPath(FILE *outfile, NODE *node);

//functions

void mkdir(void)
{
	NODE *dir, *newNode;								// Pointers to dname node and node to be created
	if (!pathname)
        {
		printf("Invalid path.\n");
		return;
	}
	dbname(pathname);								// Split given path, populates dname and bname globals
  //printf("bname: %s\tdname: %s\n", bname, dname);
	if (strcmp(pathname, "") == 0)							// Base case, if no pathname was provided, give error message and return
	{
		printf("No pathname provided.\n");
		return;
	}

	if (pathname[0] == '/')
  	dir = root; //Absolute mode
	else										// Relative mode
		dir = findDir(cwd);

	if (dir)									// If the directory node exists
	{
		if (search_child(bname, dir->childPtr))				// Check to see if dir already exists, if so print error message and exit
		{
			printf("Directory \'%s\' already exists.\n", bname);
			return;
		}
		newNode = (NODE *)malloc(sizeof(NODE));					// Otherwise, create new node, give it name and type, and add it
		strcpy(newNode->name, bname);
		newNode->type = 'D';
		insertNode(dir, newNode);
		return;
	}

	printf("Invalid path.\n");

	return;
}

// Add node to parent's LL of children given a new node. Assumes newNode already has type and name
void insertNode(NODE *parent, NODE *newNode)
{
	NODE *navNode;									// Create navigator node so parent stays put

	if (parent->childPtr)								// If parent already has a child, navigate to the end of sibling list, put new node there
	{
		navNode = parent->childPtr;
		while (navNode->siblingPtr)
			navNode = navNode->siblingPtr;
		newNode->parentPtr = parent;
		navNode->siblingPtr = newNode;
		return;
	}

	newNode->parentPtr = parent;							// Link parent and child
	parent->childPtr = newNode;

	return;
}

void splitPath(char *path)
{
	int i = 0, j = 0, baseIndex = 0, dirIndex = 0;

	// First, get the bname

	while (path[i] != 0)								// Navigate to the end of the string
		i++;

	while (path[i] != '/' && i > 0)							// Navigate to the final '/' of the path
		i--;

	baseIndex = i;									// Record index of the end of the directory for later

	if (path[i] == '/') i++;							// Move to the start of the bname if we're dealing with a path

	while (path[i] != 0)								// Copy remaining characters of bname into global bname container
	{
		bname[j] = path[i];
		i++; j++;
	}
	bname[j] = 0;								// End string with null char

	while (dirIndex < baseIndex)							// Copy characters from dir index to base index into dname
	{
		dname[dirIndex] = path[dirIndex];
		dirIndex++;
	}
	dname[i] = 0;									// Put null char to end the string

	if (strcmp(dname, "/") == 0)	return;						// If the dname is just root, leave it and return.

	dirIndex = strlen(dname) - 1; 						// Set dirIndex to the end of the string

	while (dname[dirIndex] != '/' && dirIndex > 0)				// Move backward until we reach a '/' or the beginning of the string
	{
		dirIndex--;
	}

	if (dirIndex != 0)								// If we didn't reach the beginning of the string (therefore found a '/'), get rid of everything up to that '/'
		memmove(dname, dname + dirIndex + 1, strlen(dname));

	if (dname[0] == '/')								// Finally, check to see if dname starts with a '/'. If so, remove it.
		memmove (dname, dname + 1, strlen(dname));

	return;
}

void dbname(char *path){
  char temp[128];
  strcpy(temp, path);
  strcpy(dname, dirname(temp));
  strcpy(temp, path);
  strcpy(bname, basename(temp));

  return;
}

void rmdir(void)
{
	NODE *dir;// Pointers to dname node and node to be created

        if (!pathname)
        {
                printf("Invalid path.\n");
                return;
        }

        dbname(pathname);                                                            // Split given path, populates dname and bname globals

	if (strcmp(pathname, "") == 0)                                                  // Base case, if no pathname was provided, give error message and return
        {
                printf("No pathname provided.\n");
                return;
        }

        if (pathname[0] == '/')                                                         // Absolute mode
                dir = findDir(root);
        else                                                                            // Relative mode
                dir = findDir(cwd);

        if (dir)                                                                        // If the directory node exists
        {
		dir = search_child(bname, dir->childPtr);				// At this point, dir is now the directory user is trying to remove

                if (!dir)					                        // If the directory doesn't exist, error message and return
                {
                        printf("Directory *%s* does not exist.\n", bname);
                        return;
                }

		if (strcmp(dir->name, "/") == 0)					// Make sure they're not trying to delete the root node
		{
			printf("Cannot delete root node.\n");
			return;
		}

		if (dir->type != 'D')							// Not actually sure if this is possible at this point, but check to see if the thing we're deleting is actually a directory
		{
			printf("\'%s\' is not a directory.\n", bname);
			return;
		}

		if (dir->childPtr)
		{
			printf("Directory \'%s\' has contents and cannot be removed.\n", bname);
			return;
		}

		if (strcmp(dir->name, cwd->name) == 0)					// If the dir being deleted is the cwd, move back to parent before deleting
		{
			cwd = cwd->parentPtr;
		}

		deleteNode(dir);

                return;
        }

        printf("Invalid path.\n");

        return;
}

// Deletes node from file tree
void deleteNode(NODE *node)
{
	NODE *parent, *sibling;
	char *childName;

	parent = node->parentPtr;
	childName = parent->childPtr->name;

	if (strcmp(childName, node->name) == 0)				// If the given node is the first child of it's parent, simply delete it and make parent's first child the node's next sibling
	{
		parent->childPtr = node->siblingPtr;					// Could be null, doesn't matter though
		free(node);
		return;
	}

	sibling = parent->childPtr;							// If the node isn't a direct child, first set sibling to be the parent's first child

	while (strcmp(sibling->siblingPtr->name, node->name) != 0)			// Navigate sibling to the sibling before given node
		sibling = sibling->siblingPtr;

	sibling->siblingPtr = node->siblingPtr;						// Bypass given node, sibling could be null but doesn't matter

	free(node);									// See ya, node

	return;
}

// Lists contents of current directory
void ls(void)
{
	NODE *children = cwd->childPtr;							// sibling list begins with current dir's child
	int i = 0;

	if (!children)									// Base case, if cwd has no children, let user know and return
	{
		printf("Directory empty.\n");
		return;
	}

	while (children)								// While there are children left in the list,
	{
		printf("%s\t", children->name);						// Display child, move to next sibling
		children = children->siblingPtr;
		if (i == 10)								// Formatting, creates rows of 10 items
		{
			putchar('\n');
			i = 0;
		}
		i++;
	}

	putchar('\n');

	return;
}

void cd(void)
{
	NODE *dest;

	if (strcmp(pathname, "") == 0)							// First base case, if no pathname is specified, move to root dir
	{
		cwd = root;
		return;
	}

	if (strcmp(pathname, ".") == 0) return;						// Second base case, if pathname = '.', do nothing and return

	if (strcmp(pathname, "..") == 0)						// Third base case, if pathname = "..", move to cwd's parent node (if applicable)
	{
		if (!cwd->parentPtr)
		{
			printf("No parent directory.\n");
			return;
		}

		cwd = cwd->parentPtr;

		return;
	}

	dbname(pathname);								// Split given path, populates dname and bname globals

	if (pathname[0] == '/')								// Absolute mode
		dest = findDir(root);
	else										// Relative mode
		dest = findDir(cwd);

	if (dest)									// If the dir node exists, search it for base node, move cwd to it if it exists.
	{
		dest = search_child(bname, dest->childPtr);				// See if the base node exists in the directory

		if (!dest)								// If the base node isn't there, print error and return
		{
			printf("Directory does not exist. To create one, enter: mkdir 'directoryname'.\n");
			return;
		}

		if (dest->type != 'D')							// If it is there but isn't a directory, print error and return;
		{
			printf("Given node is not a directory.\n");
			return;
		}

		cwd = dest;								// If all is well, set cwd to it and return
		return;
	}

	return;
}

void pwd(void)
{
	rpwd(cwd);//call the recursive
	putchar('\n');

	return;
}
//recursive pwd
void rpwd(NODE *node)
{
	if (!node)									// If we reach a null node, return (base case)
		return;
	rpwd(node->parentPtr);								// Move to parent node, will go all the way to root directory before printing
	printf("%s", node->name);							// On the way back, print node name
	return;
}

void creat(void)
{
	NODE *dir, *newNode;// Pointers to dname node and node to be created

        if (!pathname)
        {
                printf("Invalid path.\n");
                return;
        }

        dbname(pathname); // Split given path, populates dname and bname globals

        if (strcmp(pathname, "") == 0) // Base case, if no pathname was provided, give error message and return
        {
                printf("No pathname provided.\n");
                return;
        }

        if (pathname[0] == '/')// Absolute mode
                dir = findDir(root);
        else
                dir = findDir(cwd); // Relative mode

        if (dir)			// If the directory node exists
        {
                if (search_child(bname, dir->childPtr))// Check to see if dir already exists, if so print error message and exit
                {
                        printf("File \'%s\' already exists.\n", bname);
                        return;
                }
                newNode = (NODE *)malloc(sizeof(NODE)); // Otherwise, create new node, give it name and type, and add it
                strcpy(newNode->name, bname);
                newNode->type = 'F';
                insertNode(dir, newNode);
                return;
        }

        printf("Invalid path.\n");


	return;
}

void rm(void)
{
	NODE *file, *parent; // Pointers to dname node and node to be created

	if (!pathname)
        {
                printf("Invalid path.\n");
                return;
        }

        dbname(pathname); // Split given path, populates dname and bname globals

        if (strcmp(pathname, "") == 0) // Base case, if no pathname was provided, give error message and return
        {
                printf("No pathname provided.\n");
                return;
        }

        if (pathname[0] == '/')                                                         // Absolute mode
                file = findDir(root);
        else                                                                            // Relative mode
                file = findDir(cwd);

        if (file)                                                                        // If the directory node exists
        {
                file = search_child(bname, file->childPtr);                               // At this point, file is now the file user is trying to remove

                if (!file)                                                               // If the file doesn't exist, error message and return
                {
                        printf("File\'%s\' does not exist.\n", bname);
                        return;
                }

                if (file->type != 'F')                                                   // Not actually sure if this is possible at this point, but check to see if the thing we're deleting is actually a directory
                {
                        printf("\'%s\' is not a file.\n", bname);
                        return;
                }

                deleteNode(file);

		return;
        }

        printf("Invalid path.\n");

        return;
}

void quit(void)
{
  save();
	exit(0);
	return;
}


void menu(void)
{
  printf("======================================================\n"
          "mkdir rmdir ls cd pwd rm creat reload save menu quit \n"
          "=====================================================\n");

	return;
}

void reload(void)
{
	FILE *infile;
	char type, trash;

	if (strcmp(pathname, "") == 0)
		return;


	infile = fopen(pathname, "r+");

	if (!infile)
	{
		printf("Input file not found.\n");
		return;
	}

	fgets(line, 128, infile); //get the first two lines
	fgets(line, 128, infile);
  //fgets(line, 128, infile);

	// Scan in the first line containing root directory. We don't need it since we will already have one
        fscanf(infile, "%c", &type);                                    // Scan the type char
        fscanf(infile, "%c", &trash);                                   // Scan the \t, trash it
        fscanf(infile, "%s", pathname);                                 // Scan the pathname
        fscanf(infile, "%c", &trash);                                   // Scan the newline char, trash it

	do									// Each iteration processes one line
	{
		if (feof(infile)) break;

		memReset();							// Reset between formations

		fscanf(infile, "%c", &type);					// Scan the type char
		fscanf(infile, "%c", &trash);					// Scan the \t, trash it
		fscanf(infile, "%s", pathname);					// Scan the pathname
		fscanf(infile, "%c", &trash);					// Scan the newline char, trash it

		pathname[strlen(pathname) - 1] = 0;				// Trim the / at the end of the pathnamei
    //printf("Pathname is: %s\n", pathname);
		if (type == 'D')
		{
			mkdir();
			continue;
		}
		if (type == 'F')
		{
			creat();
			continue;
		}

	} while (type && pathname);

	return;
}

void save(void)
{
	FILE *outfile;

	if (strcmp(pathname, "") == 0)
		return;

	outfile = fopen(pathname, "w+");

	fprintf(outfile, "Type\tPath\n");
	fprintf(outfile, "-----  -----------------\n");

	rsave(outfile, root); //call to recursive save

	fclose(outfile);

	return;
}
//recursive save
void rsave(FILE *outfile, NODE *node)
{
	NODE *child;

	if (!node) return;

	fprintf(outfile, "%c\t", node->type);

	rPrintPath(outfile, node);
	fprintf(outfile, "\n");

	child = node->childPtr;

	while(child)
	{
		rsave(outfile, child);
		child = child->siblingPtr;
	}

	return;
}

void rPrintPath(FILE *outfile, NODE *node)
{
        // Note: Same logic as rpwd function, but with added formatting at the end for input line structure
        if (node->parentPtr)
                rPrintPath(outfile, node->parentPtr);

        fprintf(outfile, "%s", node->name);
        if (strcmp(node->name, "/") != 0 && strcmp(node->name, root->name) != 0){
          fprintf(outfile, "/");
        }        // If the node is any node other that root node or cwd, print a '/' after node

}


//help

// Find command function index based on a given command string
int findCmd(char *command)
{
	int i = 0;

	while (cmd[i])
	{
		if (strcmp(command, cmd[i]) == 0)
			return i;
		i++;
	}

	return -1;
}

//this is a modified tokenize
NODE *findDir(NODE *node)
{
	char *token, *dirPath;									// Temp string container to hold pathname tokens
	int i = 1, j = 0;

	if (strcmp(dname, ".") == 0) return node;					// dirname returns '.' for a '/ in the directory'

	if (hasPath()) //if theere is actually a path here
	{
		dirPath = malloc(sizeof(char) * strlen(pathname));
		strcpy(dirPath, pathname);						// Make a duplicate of pathname, then remove the last token from it
		j = strlen(dirPath) - 1;

		while (dirPath[j] != '/') j--; //look fot a '/'
		dirPath[j] = 0;

		token = strtok(dirPath, "/");

		do							// For each token in path name,
		{
			node = search_child(token, node->childPtr);					// Find sibling node with the given node name
			if (node)								// If it exists, continue to next token if thje node was directory
			{
				if (node->type != 'D')
				{
					printf("Node \'%s\' is not a directory.\n", token);
					return 0;
				}
			}
			else return 0;
			i++;
		} while (token = strtok(0, "/"));
	}

	if (strcmp(node->name, dname) != 0) return 0; //if its not the same, return

	return node;
}

// Finds the node amongst all siblings with the given directory name
NODE *search_child(char *dname, NODE *child)
{
	while (child)									// while siblings left to check
	{
		if (strcmp(child->name, dname) == 0)					// See if sibling matches given name
		{
			return child;
		}
		child = child->siblingPtr;
	}
	return 0;
}

// Initialization function, creates root node to begin an empty file tree
void initialize(){
  root = malloc(sizeof(NODE));
  strcpy(root->name, "/");
  root->type = 'D';
  root->childPtr = root->siblingPtr = root->parentPtr = NULL;

  cwd = root;
  return;
}


// Display the path of a given node
void displayPath(NODE *node)
{
	// Note: Same logic as rpwd function, but with added formatting at the end for input line structure
	if (node->parentPtr)
		displayPath(node->parentPtr);

	printf("%s", node->name);
	if (strcmp(node->name, "/") != 0 && strcmp(node->name, cwd->name) != 0)		// If the node is any node other that root node or cwd, print a '/' after node
		putchar('/');
}

// Checks to see if the pathname has any '/' characters in it
int hasPath(void)
{
	int i = 0;

	while (pathname[i])
	{
		if (pathname[i] == '/')	return 1;
		i++;
	}
	return 0;
}

// Checks to see if the user input line has a space in it
int hasSpace(void)
{
	char *c = line;
	int i = 0;

	while (c[i])
	{
		if (c[i] == ' ')
			return 1;
		i++;
	}
	return 0;
}

// Resets pathname and dname so that their use in command functions don't interfere with future uses
void memReset(void)
{
	memset(line, 0, 128);
	memset(command, 0, 16);
	memset(pathname, 0, 64);
	memset(dname, 0, 64);
	memset(bname, 0, 64);

	return;
}

void displayNode(NODE *node)
{
	printf("*** Node Display ***\n");
	if (!node)
	{
		printf("Node is null!\n");
		return;
	}

	printf("Node name: %s\n", node->name);
	printf("Node type: %c\n", node->type);
	printf("Parent: %s\n", node->parentPtr->name);
	printf("Sibling: %s\n", node->siblingPtr->name);
	printf("First child: %s\n", node->childPtr->name);

	return;
}


//main
int main(void)
{
  	char *stemp;

    //Initialize root node
  	initialize();
    // Set current directory to root


    // Loop continuously, program will halt when user calls quit function
  	while(1)
  	{
      // Reset pathname and dname strings so previous commands won't interfere with new ones
  		memReset();

  		// Get user input
  		displayPath(cwd);
      printf("\nCommand :");
      fgets(line, 128, stdin); //gets at most 128 chars from stdin
      line[strlen(line)-1] = 0; //kills the \n at theend of the line
      sscanf(line, "%s %s", command, pathname);
      int id = findCmd(command);
      // If the input line has a space in it (meaning multiple fields)
  		{
  		switch (id){
  			case 0:
  				//printf("pathname: %s\ndname: %s\nbname: %s\n", pathname, dname, bname);
  				mkdir();
  				break;
  			case 1:
  				rmdir();
  				break;
  			case 2:
  				ls();
  				break;
  			case 3:
  				cd();
  				break;
  			case 4:
  				pwd();
  				break;
  			case 5:
  				creat();
  				break;
  			case 6:
  				rm();
  				break;
  			case 7:
					strcpy(pathname, "DefaultSaveFile");
					save();
  				quit();
  				break;
  			case 8:
  				menu();
  				break;
  			case 9:
  				reload();
  				break;
  			case 10:
  				save();
  				break;
  			default:
  				printf("Command \' %s\'  not found.\n", command);
  				break;
  		}
  	putchar('\n');
  	}
  }
}
