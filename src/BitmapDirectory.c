#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include <X11/Xlibint.h>

#include "general.h"
#include "errcheck.h"
#include "BitmapDirectory.h"

/*
  given a name, finds the pBitmapDirectory in the list.
*/
static pBitmapDirectory FindBitmap(
      String name,
      pBitmapDirectory bd) {
  pBitmapDirectory node;
  for(node = bd; node; node = node->next)
    if( !strcmp(name, node->bitmap_entry.name) )return node;
  return NULL;
}


static pBitmapDirectory AddFile(
      Widget w,
      String file,
      String name,
      pBitmapDirectory bd) {
  int dummy;
  tBitmapEntry bitmap_entry;
  pBitmapDirectory entry;
  Window win = XRootWindowOfScreen(XtScreen(w));

  if(XReadBitmapFile(XtDisplay(w), win, file,
		     &bitmap_entry.width, &bitmap_entry.height,
		     &bitmap_entry.bitmap,
		     &dummy, &dummy) !=BitmapSuccess) 
    return bd; /* Ignore invalid bitmap files */
  if(!(entry = FindBitmap(name, bd))) {
    /* create new entry */
    entry = NEW(tBitmapDirectory);
    entry->next = bd;
    bd = entry;
  } /* else replace existent entry! */
  entry->bitmap_entry = bitmap_entry;
  entry->bitmap_entry.name = strdup(name);
  return bd;
}

tBitmapDirectory* BDAddDirectory(
      Widget w,
      String directory,
      tBitmapDirectory *bd) {
  DIR *dir;
  struct dirent *dp;
  struct stat statbuf;
  char fullfilename[BUFSIZE];

  dir = (DIR *)errcheck_ptr(opendir(directory), NULL, directory);
  for(dp = readdir(dir); dp; dp = readdir(dir))
    if(dp->d_name[0] != '.') { /* Ignore files starting with '.' */
      sprintf(fullfilename, "%s/%s", directory, dp->d_name);
      stat(fullfilename, &statbuf);
      if(statbuf.st_mode & S_IFDIR)
	/* Subdirectory, so add it... */
	bd = BDAddDirectory(w, fullfilename, bd);
      else
	bd = AddFile(w, fullfilename, dp->d_name, bd);
    }
  (void)errcheck((int)closedir(dir), -1, directory);
  return(bd);
}

/*
  returns the entry of the specified name
*/
void BDFindBitmap(
      String name,            /* name of entry to search for */
      tBitmapEntry **entry,   /* return param */
      pBitmapDirectory bd) {  /* directory to search */
  pBitmapDirectory d;
  if(  (d = FindBitmap(name, bd))  ) {
    *entry = &(d->bitmap_entry);
  } else
    *entry = NULL;
}

/*
  returns next entry and cycles when reaches the end ? ..or ?
*/
void BDBitmaps(
     tBitmapDirectory **entry,
     tBitmapEntry **bitmap_entry,
     tBitmapDirectory *bd) {
  if(*entry)
    *entry = (*entry)->next;
  else
    *entry = bd;
  if(*entry) {
    *bitmap_entry = &((*entry)->bitmap_entry);
  } else
    *bitmap_entry = NULL;
}

