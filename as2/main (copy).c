#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>


typedef struct my_node{
  char name[64];
  char type;
  struct Node *childPtr, *siblingPtr, *parentPtr;
}Node;

//globals
Node *root, *cwd, *start;
char line[128];
char command[16], pathname[64];
char dname[64], bname[64];
int cond = 1;
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm",
"reload", "save", "quit", "menu", NULL};
char *names[100];
char *pwdNames[100];
Node *pwdNodes[100];


//initalize the root directory
void initialize(){
  root = malloc(sizeof(Node));
  strcpy(root->name, "/");
  root->type = 'D';
  root->childPtr = root->siblingPtr = root->parentPtr = NULL;

  cwd = root;
}

//get the command
int findcmd(char *command){
  int i = 0;
  while(cmd[i]){
    if (!strcmp(command, cmd[i]))
      return i;
    i++;
  }
  return -1; //couldnt find it
}

NODE *findDir(Node *node){
	char *token, *dirPath;									// Temp string container to hold pathname tokens
	int i = 1, j = 0;

	if (strcmp(dirname, "") == 0) return node;					// If there is no dirname, that means dir is whatever node was given

	if (hasPath())
	{
		dirPath = malloc(sizeof(char) * strlen(pathname));
		strcpy(dirPath, pathname);						// Make a duplicate of pathname, then remove the last token from it
		j = strlen(dirPath) - 1;

		while (dirPath[j] != '/') j--;
		dirPath[j] = 0;

		token = strtok(dirPath, "/");

		do							// For each token in path name,
		{
			node = search_child(token, node->childPtr);					// Find sibling node with the given node name
			if (node)								// If it exists, continue to next token if sought node was directory
			{
				if (node->type != 'D')
				{
					printf("Node *%s* is not a directory.\n", token);
					return 0;
				}
			}
			else return 0;
			i++;
		} while (token = strtok(0, "/"));
	}

	if (strcmp(node->name, dirname) != 0) return 0;

	return node;
}

int dbname(char *path){
  char temp[128];
  strcpy(temp, path);
  strcpy(dname, dirname(temp));
  strcpy(temp, path);
  strcpy(bname, basename(temp));

  return 0;
}

int tokenize(char *pathname){
  int i = 0;
  //printf("in tokenize\n");
  char *s;
  s = strtok(pathname, "/");  // first call to strtok()
  while(s){
    //printf("%s ", s);
    names[i] = s;
    i++;
    s = strtok(0, "/");  // call strtok() until it returns NULL
  }
  return i;
}

Node * search_child(Node* parent, char *name){
  //printf("Searching for %s in %s\n", name, parent->name);
  if(parent == NULL){
    return NULL;
    }

  if(strcmp(parent->name, name) == 0){
    //  printf("found %s\n", name);
      return parent;
  }

  if(parent != NULL){
  Node *ch = search_child(parent->childPtr, name);
  if(ch != NULL)
    return ch;
  else
    return  search_child(parent->siblingPtr, name);
  }


  return NULL;

}

Node *path2node(char *pathname){
  //return thoe node pointer of a pathname, or 0 if the node does not exist
  int n, i;
  char path[64];

  strcpy(path, pathname);
  //printf("PATHNAME IS: %s\n",pathname);
  if(pathname[0] = '/'){
      start = root;
  }else{
    start = cwd;
  //  printf("IN THE ELSE\n");
  }
  //printf("start is: %s \n", start->name);

  Node *p = start;
  n = tokenize(path);
  for(i = 0; i < n; i++){
    p = search_child(p, names[i]);
    if (p==0) return 0; //if name[i] does not exist
  }
  return p;
}

int mkdir(char *pathname){
  Node *tempNode;
  Node *parentNode;
  dbname(pathname);
  //printf("dname: %s - bname %s\n", dname, bname);
  if(strcmp(dname, ".") == 0){
    strcpy(dname,cwd->name);
  }
  parentNode = path2node(dname);
  //printf("parentnode name: %s\n", dname);

  if(parentNode != NULL){
    if(parentNode->type != 'D'){
      printf("Not a directory\n");
      return -1;
    }
    char tempName[64];
    strcpy(tempName, bname);
    tempNode = path2node(tempName);
    if(tempNode == NULL){
    //  printf("Creating new node.\n");
      tempNode = malloc(sizeof(Node));
      tempNode->type = 'D';
      printf("bname is: %s\n",bname);
      strcpy(tempNode->name, bname);
      tempNode->parentPtr = parentNode;
      tempNode->childPtr = tempNode->siblingPtr = NULL;

      if(parentNode->childPtr == NULL){
        parentNode->childPtr = tempNode;
      }else if(parentNode->siblingPtr == NULL){
      //  printf("%s is sibling of %s\n",tempNode->name, parentNode->name);
        parentNode->siblingPtr = tempNode;
      }else
        return -1;
      return 0;
    }
    printf("Error. already exists\n");
      return -1;
    }

}

int creat(char *pathname){
  Node *tempNode;
  Node *parentNode;
  dbname(pathname);
  //printf("dname: %s - bname %s\n", dname, bname);
  if(strcmp(dname, ".") == 0){
    strcpy(dname,cwd->name);
  }
  parentNode = path2node(dname);
  //printf("parentnode name: %s\n", dname);

  if(parentNode != NULL){
    if(parentNode->type != 'D'){
      printf("Not a directory\n");
      return -1;
    }
    char tempName[64];
    strcpy(tempName, bname);
    tempNode = path2node(tempName);
    if(tempNode == NULL){
      //printf("Creating new node.\n");
      tempNode = malloc(sizeof(Node));
      tempNode->type = 'F';
      //printf("bname is: %s\n",bname);
      strcpy(tempNode->name, bname);
      tempNode->parentPtr = parentNode;
      tempNode->childPtr = tempNode->siblingPtr = NULL;

      if(parentNode->childPtr == NULL){
        parentNode->childPtr = tempNode;
      }else if(parentNode->siblingPtr == NULL){
        parentNode->siblingPtr = tempNode;
      }else
        return -1;
      return 0;
    }
    printf("Error. already exists\n");
      return -1;
    }

  /*
  Node *tempNode = malloc(sizeof(Node));
  dbname(pathname);

  tempNode = search_child(root, dname);

  if(tempNode != NULL){
    if(tempNode->type != 'D'){
      printf("Not a directory\n");
      return -1;
    }
    tempNode = search_child(tempNode, bname);
    if(tempNode != NULL){ //already exists
      printf("directory already exists. \n");
      return -1;
    }else{
      //add new directory
      printf("not found, need to add dir for %s. Inserting...\n", bname);
      Node *parent = path2node(dname);
      printf("Parent name: %s\n",parent->name);
      Node *temp2 = malloc(sizeof(Node));
      temp2->type = 'F';
      strcpy(temp2->name, bname);
      temp2->childPtr = temp2->siblingPtr = NULL;
      temp2->parentPtr = parent;
      if(parent->childPtr == NULL){
        parent->childPtr = temp2;
      }else if(parent->siblingPtr == NULL){
        parent->siblingPtr = temp2;
      }else{
        return -1;
      }

      return 0;
    }
  }else{
    printf("tempNode Name %s\n", tempNode->name);
    printf("directory does not exist\n");
    return -1;
  }
  */

}

int rm(char *pathname){
  if(pathname[0] == '/') {
    start = root;
  }else{
    start = cwd;
  }
  Node *temp = path2node(pathname);
  if(temp != NULL){ //it exists
    Node *parent = temp->parentPtr;
    //printf("Parent is: %s\n", parent->name);
    if(temp->type != 'F'){
      printf("error removing, not a file.\n");
      return -1;
    }else{
      //both child and sibling are null, can remove
      Node *pCh = parent->childPtr;
      Node *pSb = parent->siblingPtr;
      if(pCh == temp){
        //printf("temp is a child\n");
        parent->childPtr = NULL;
        temp->parentPtr = NULL;
        free(temp);
        return 0;
      }else if(pSb == temp){
        //printf("temp is a sibling\n");
        parent->siblingPtr = NULL;
        temp->parentPtr = NULL;
        free(temp);
        return 0;
      }
    }
  }else{
    printf("Error. Dir does not exist\n");
  }
}

int rmdir(char *pathname){
  //printf("in rmdir\n");
  if(pathname[0] == '/') {
    start = root;
  }else{
    start = cwd;
  }
  Node *temp = path2node(pathname);
  if(temp != NULL){ //it exists
    Node *parent = temp->parentPtr;
    //printf("Parent is: %s\n", parent->name);
    if(temp->childPtr != NULL || temp->siblingPtr != NULL){
      printf("error removing dir, not empty.\n");
      return -1;
    }else{
      //both child and sibling are null, can remove
      Node *pCh = parent->childPtr;
      Node *pSb = parent->siblingPtr;
      if(pCh == temp){
        //printf("temp is a child\n");
        parent->childPtr = NULL;
        temp->parentPtr = NULL;
        free(temp);
        return 0;
      }else if(pSb == temp){
        //printf("temp is a sibling\n");
        parent->siblingPtr = NULL;
        temp->parentPtr = NULL;
        free(temp);
        return 0;
      }
    }
  }else{
    printf("Error. Dir does not exist\n");
  }
}

int cd(char *pathname){
  //printf("pathname : %s\n", pathname);
  Node *p;
  if(!strcmp(pathname, "..")){
    if(cwd->parentPtr != NULL){
      cwd = cwd->parentPtr;
      //printf("New CWD is at: %s\n", cwd->name);
      return 0;
    }else{
      printf("Error. Nowhere to cd to. ");
      return -1;
    }
  }else{
    dbname(pathname);
    if(pathname[0] == '/'){
        p = path2node(root);
    }else{
      p = path2node(cwd);
    }

    if(p){
      p = search_child(p,bname);

      if(!p){
        printf("Error. Directory does not exist\n");
        return -1;
      }

      if(p->type !='D'){
        printf("Node is not Directory.\n");
        return -1;
      }

      cwd = p;
      return 0;

    }
    /*
    printf("\np name is: %s\n",p->name);
    if(p != NULL && p->type == 'D'){//exists and is dir
      cwd = p;
      printf("New CWD is at: %s\n", cwd->name);
      return 0;
    }
    return -1;*/
  }


}

void ls(void)
{
	Node *children = cwd->childPtr;							// sibling list begins with current dir's child
	int i = 0;

	if (!children)									// Base case, if cwd has no children, let user know and return
	{
		printf("Directory empty.\n");
		return;
	}

	while (children)								// While there are children left in the list,
	{
		printf("%c\t%s\n",children->type, children->name);						// Display child, move to next sibling
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

void printNodes(Node *p){
  if(p != NULL){
    if(p != cwd)
      printf("%s : %c \n",p->name, p->type);
    printNodes(p->childPtr);
    printNodes(p->siblingPtr);
  }

}

void pwd(Node *p, int i){
  if(p != NULL){
    pwdNames[i] = p->name;
    //printf("Name is: %s\n"names)
    //printf("%s/",p->name);
    pwd(p->parentPtr, i+1);
  }else{
    for(i = i-1; i>=0; i--){
      //if(strcmp(pwdNames[i], "/") == 0 )
          printf("%s  ", pwdNames[i]);
    }
  }

}

void pwd2(Node *p, int i, FILE *f){
  if(p != NULL){
    pwdNodes[i] = p;
    //printf("Name is: %s\n"names)
    //printf("%s/",p->name);
    pwd2(p->parentPtr, i+1, f);
  }else{
    for(i = i-1; i>=0; i--){
        //printf("here\n");
        fprintf(f,"%c      %s\n",pwdNodes[i]->type, pwdNodes[i]->name);
    }
  }
  //return pwdNodes;
}

void save(char *fileName){
  FILE *outfile;
  if(strcmp(fileName, "") == 0)
    return;

  outfile = fopen(fileName, "w+");

  fprintf(outfile, "Type\tPath\n");
  fprintf(outfile, "----  --------------------");

  rsave(outfile, root);

  fclose(outfile);
}

void rsave(FILE *outfile, Node *node){
  Node *child;

  if(!node) return;

  fprintf(outfile, "%c\t", node->type);

  rPrintPath(outfile, node);
  fprintf(outfile, "\n");

  child = node->childPtr;

  while(child){
    rsave(outfile, child);
    child = child->siblingPtr;
  }

  return;

}

void rPrintPath(FILE *outfile, Node *node){

  if(node->parentPtr)
    rPrintPath(outfile, node->parentPtr);

  fprintf(outfile, "%s", node->name);
  if(strcmp(node->name, "/") != 0 && strcmp(node->name, cwd->name) !=0)
      fprintf(outfile, "/\n");

}






int main(int argc, char *argv){
  initialize();
  while(cond){
    //clear the previous globals
    memset(&pathname, 0, sizeof(pathname));
    memset(&dname, 0, sizeof(dname));
    memset(&bname, 0, sizeof(bname));
    printf("Command : ");
    fgets(line, 128, stdin); //gets at most 128 chars from stdin
    line[strlen(line)-1] = 0; //kills the \n at theend of the line
    sscanf(line, "%s %s", command, pathname);
    int index = findcmd(command);


    int key;
    switch(index) {
      case 0 : //mkdir

        if(strlen(pathname) == 0){
          printf("Pathame is empty. Try again.\n");
          break;
        }else if(!strcmp(pathname, "/")){
          printf("Error. Cannot create a second root directory.\n");
          break;
        }
      //  printf("Command is mkdir, pathname is: %s\n",pathname );

        if(mkdir(pathname) == 0){
          //printf("mkdir success.\n");

        }else{
          printf("Error creating directory.\n");
        }
        break;
      case 1 : //rmdir

        if(strlen(pathname) == 0){
          printf("Pathame is empty. Try again.\n");
          break;
        }
        if(rmdir(pathname) == 0){
          //printf("rmdir success.\n");
        }else{
          printf("error rmdir.\n");
        }
        break;
      case 2 : //ls
        ls();
        break;
      case 3:

        if(strlen(pathname) == 0){
          printf("Pathame is empty. Try again.\n");
          break;
        }else{
          if(cd(pathname) == 0){
            //printf("cd success\n");
            break;
          }
          printf("error in cd to \'%s\'\n", pathname);
        }
        break;
      case 4: //pwd
        //printf("%s\n", cwd->name);
        pwd(cwd, 0);
        printf("\n");
        break;
      case 5: //creat
        creat(pathname);
        break;
      case 6: //rm
        if(strlen(pathname) == 0){
          printf("Pathame is empty. Try again.\n");
          break;
        }
        rm(pathname);

        break;
      case 7: //reload

        break;
      case 8 : //save

        save(pathname);
        break;
      case  9: //quit

        //save here
        cond = 0;
        break;
      case 10 :
        printf("======================================================\n"
                "mkdir rmdir ls cd pwd rm creat reload save menu quit\n"
                "=====================================================\n");
        break;
      default :
      printf("Error: Command\'%s\' not found. Type \'menu\' for help. \n", command);
    }
  }
  return 0;
}
