/* DEP - Dingoo Emulation Pack for Dingoo A320
 *
 * Copyright (C) 2012-2013 lion_rsm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
 
#include "framework.h"
#include "launcher.h"
#include "stdio.h"

using namespace fw;
using namespace hw;

#define CONFIG_FILENAME  "launcher.conf"
#define SECTIONS_PATH    "sections"
#define SKINS_PATH       "skins"
#define WALLPAPER_PATH   "wallpapers"
#define IMGS_PATH        "imgs"
#define ICONS_MIN_ALLOC    50
#define SKIN_NAME_DEFAULT  "Default"
#define SECTION_ITEMS_PER_ROW               5
#define SECTION_ITEMS_PER_COL               4

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
typedef struct {
	char path[HW_MAX_PATH];   //required for checking if icon already loaded
	gfx_texture *icon;
} Icon_t;

typedef struct {
	char name[256];
	char path[HW_MAX_PATH];
	char filename[256];
	int icon;
	char exec[HW_MAX_PATH];
	int x;
	int y;
} SectionItem_t;

typedef struct {
	SectionItem_t **item;
	int count;
} SectionItemList;

typedef struct {
	char name[256];
	char path[HW_MAX_PATH];
	char filename[256];
	int icon;
	SectionItemList *items;
} Section_t;

typedef struct {
	Section_t **section;
	int count;
} SectionList;

static gfx_font* mpCurrFont;
static gfx_texture *mpCurrWallPaper;
static char mWallpaperName[HW_MAX_PATH];
static char mSkinName[HW_MAX_PATH];
static gfx_texture *mpCurrTopBar;
static Icon_t **mpIcons;
static int mIcon=0;
static int mIconCount=ICONS_MIN_ALLOC;
static Section_t *mpSection=NULL;
static int mPosX=0;
static int mPosY=0;
static int mStartY=0;
static int m1stDisplayedItem=0;
static int mItemInUse[SECTION_ITEMS_PER_COL*SECTION_ITEMS_PER_ROW];
static char mSectionsPath[HW_MAX_PATH];
static char mSkinsPath[HW_MAX_PATH];
static char mSkinPath[HW_MAX_PATH];
static char mSkinDefaultPath[HW_MAX_PATH];
static char mWallpapersPath[HW_MAX_PATH];
static SectionList *mpSections=NULL;

void save_config()
{
	char filename[HW_MAX_PATH];
	char line[HW_MAX_PATH*2];

	strcpy(filename, fsys::getEmulPath());
	fsys::combine(filename, CONFIG_FILENAME);

	FILE *f = fopen(filename, "w");
	if(f == NULL)
	{
		debug::printf("Failed to open config file:%s\n", filename);
		return;
	}

	sprintf(line,"skin=%s\n",mSkinName);
	fputs(line, f);

	sprintf(line,"wallpaper=%s\n",mWallpaperName);
	fputs(line, f);
     
	fclose(f);
}

void launcher::SetWallpaper(char *filename)
{
	if(mpCurrWallPaper!=NULL) free(mpCurrWallPaper);
	mpCurrWallPaper=NULL;
	strcpy(mWallpaperName, filename);
	char path[HW_MAX_PATH];
	strcpy(path, mWallpapersPath);
	fsys::combine(path, filename);
	debug::printf((char*)"Wallpaper:%s\n",path);
	mpCurrWallPaper = video::tex_load(path);
	save_config();
}

char *launcher::GetWallpapersPath()
{
	return mWallpapersPath;
}

char *launcher::GetSkinsPath()
{
	return mSkinsPath;
}

char *launcher::GetDefaultSkinPath()
{
	return mSkinDefaultPath;
}
//
static void GetDisplayName(char *filename, char *name)
{
	//Find 1st underscore - this is the delimiter for index and filename
	int d=-1;
	for(int x=0; x<strlen(filename); x++)
	{
		if(filename[x]=='_')
		{
			d=x;
			break;
		}
	
	}
	
	if(d<=0)
	{
		//No delimiter - copy the whole lot
		strcpy(name, filename);
	}
	else
	{
		strcpy(name,filename+d+1);
	}

}

static void SwapSectionItem(SectionItem_t *itemFrom, SectionItem_t *itemTo)
{
	SectionItem_t temp;

	//Copy salFrom to temp entry
	memcpy(&temp, itemFrom, sizeof(SectionItem_t));
	memcpy(itemFrom, itemTo, sizeof(SectionItem_t));
	memcpy(itemTo,&temp,sizeof(SectionItem_t));
}

static void SwapSection(Section_t *sectionFrom, Section_t *sectionTo)
{
	Section_t temp;

	//Copy salFrom to temp entry
	memcpy(&temp, sectionFrom, sizeof(Section_t));
	memcpy(sectionFrom, sectionTo, sizeof(Section_t));
	memcpy(sectionTo,&temp,sizeof(Section_t));
}

static void SortSections(SectionList *sections, int count)
{
	for(int a=0;a<count;a++)
	{
		int lowIndex=a;		
		for(int b=a+1;b<count;b++)
		{
			if (menu::string_compare(sections->section[b]->filename, sections->section[lowIndex]->filename) < 0)
			{
				//this index is lower
				lowIndex=b;
			}
		}
		//lowIndex should index next lowest value
		SwapSection(sections->section[lowIndex],sections->section[a]);
	}
}

static void SortSectionItems(SectionItemList *items, int count)
{
	for(int a=0;a<count;a++)
	{
		int lowIndex=a;		
		for(int b=a+1;b<count;b++)
		{
			if (menu::string_compare(items->item[b]->filename, items->item[lowIndex]->filename) < 0)
			{
				//this index is lower
				lowIndex=b;
			}
		}
		//lowIndex should index next lowest value
		SwapSectionItem(items->item[lowIndex],items->item[a]);
	}
}

int StringLen(char *str)
{
	int x=0;
	while(1)
	{
		if(str[x] == 0) break;
		if(str[x] == '\r') break;
		if(str[x] == '\n') break;
		
		x++;
	
	}
	return x;
}

bool StringSplit(char *str, char delimiter, char *part1, char *part2)
{
	int len=StringLen(str);
	for(int x=0; x<len; x++)
	{
		if(str[x]==delimiter)
		{
			for(int y=0;y<x;y++)
			{
				part1[y]=str[y];
			}
			part1[x]=0;
			for(int y=x+1;y<len;y++)
			{
				part2[y-(x+1)]=str[y];
			}
			part2[len-x-1]=0;
			return true;
		}
	}
	return false;
}

int LoadIcon(char *name, char *path)
{
	int icon=mIcon;
	debug::printf("LoadIcon. %s %s\n", name, path);
	mpIcons[mIcon] = (Icon_t*)malloc(sizeof(Icon_t));
	if(mpIcons[mIcon] == NULL)
	{
		debug::printf((char*)"Failed to alloc memory\n");
		return -1;
	}
	strcpy(mpIcons[mIcon]->path, name);
	
	mpIcons[mIcon]->icon = video::tex_load(path);
	if(mpIcons[mIcon]->icon==NULL)
	{
		free(mpIcons[mIcon]);
		debug::printf((char*)"Failed to load png\n");
		return -1;
	}

	mIcon++;
	if(mIcon>mIconCount)
	{
		//Need more space
		mIconCount+=ICONS_MIN_ALLOC;
		mpIcons=(Icon_t**)realloc((void*)mpIcons, sizeof(Icon_t*)*mIconCount);
	}

	return icon;
}

bool LoadSectionItem(char *path, char *filename, SectionItem_t *item)
{
	/*
	Open file 
	read line by line
	
	title=Exit
	description=Exit GMenu2X to the official frontend
	icon=skin:icons/exit.png
	exec=/usr/gp2x/gp2xmenu
	params=--disable-autorun
	
	Check if icon already loaded
	
	*/
	strcpy(item->filename, filename);
	FILE    *f;
    	char    line[256];
    	char itemFilename[HW_MAX_PATH];
    	char name[HW_MAX_PATH];
	char value[HW_MAX_PATH];
	bool valid=true;
	strcpy(itemFilename, path);
	fsys::combine(itemFilename, filename);
     
	f = fopen(itemFilename, "r");
	if(f == NULL)
		return false;

	while(fgets(line, 256, f)){
		//Need to split line by delimiter =
		if(!StringSplit(line,'=',name,value))
		{
			debug::printf("Invalid content in %s.  Content:%s\n", itemFilename, line);
			valid=false;
			break;
		}
		debug::printf("name:%s  value:%s\n", name, value);
		if(menu::string_compare(name,"title")==0)
		{
			strcpy(item->name, value);
		}
		else if(menu::string_compare(name,"exec")==0)
		{
			strcpy(item->exec, value);
		}
		else if(menu::string_compare(name,"icon")==0)
		{
			strcpy(item->path, value);
			int found=-1;
			//Scan icon cache - do we have this one already
			for(int i=0;i<mIcon;i++)
			{
				if(menu::string_compare(item->path, mpIcons[i]->path)==0)
				{
					found = i;

					break;
				}
			}
			
			if(found>0)
			{
				item->icon = found;
				debug::printf("icon found in cache: %d  %s\n", found, item->path);
			}
			else
			{
				//Need to load new icon
				char iconFilename[HW_MAX_PATH];
				char iconDefaultFilename[HW_MAX_PATH];
				if(StringSplit(item->path, ':', name, value))
				{
					//Relative Skin ref path
					strcpy(iconFilename, mSkinPath);
					fsys::combine(iconFilename, value);
					strcpy(iconDefaultFilename, mSkinDefaultPath);
					fsys::combine(iconDefaultFilename, value);
				}
				else
				{
					//Direct path
					strcpy(iconFilename,item->path);
					strcpy(iconDefaultFilename,item->path);
				}
				int icon = LoadIcon(item->path, iconFilename);
				if(icon<0)
				{
					debug::printf((char*)"Failed to load icon\n");
					
					//Try default skin
					icon = LoadIcon(item->path, iconDefaultFilename);
					if(icon<0)
					{
						debug::printf((char*)"Failed to load default icon as well\n");
					}
				}
				
				item->icon = icon;
			}
		}
	}
     
	fclose(f);
	return valid;
}

bool LoadSectionItems(Section_t *section)
{
	debug::printf("LoadSectionItems start:%s\n", section->path);
	//Get Number of directory in sections directory
	int fileCount=0;
	if(!fsys::directoryItemCountType(section->path, MENU_FILE_TYPE_FILE, &fileCount))
	{
		debug::printf((char*)"Failed to get count\n");
		return false;
	}
	
	debug::printf((char*)"File Count: %d\n", fileCount);
	
	int sectionMemSize = sizeof(SectionItemList)
	                   + (fileCount * sizeof(SectionItem_t*))
	                   + (fileCount * sizeof(SectionItem_t));

	if(section->items == NULL)
	{
		section->items = (SectionItemList*)malloc(sectionMemSize);
	}
	
	memset(section->items,0,sectionMemSize);
	debug::printf("LoadSectionItems start2:%s\n", section->path);
	//Now setup pointers to each section item
	char *entryIdxMem=((char*)section->items)+sizeof(SectionItemList);
	char *entryMem=(char*)entryIdxMem+(sizeof(SectionItem_t*)*fileCount);

	section->items->item = (SectionItem_t**)entryIdxMem;
	for(int x=0;x<fileCount;x++)
	{
		section->items->item[x]=(SectionItem_t*)entryMem;
		entryMem+=sizeof(SectionItem_t);
	}

	MENU_DIR d;
	fileCount=0;
	if (fsys::directoryOpen((char*)section->path, &d))
	{
		//Dir opened, now stream out details
		MENU_DIRECTORY_ENTRY dir;
		while(fsys::directoryRead(&d, &dir))
		{
			if(dir.type==MENU_FILE_TYPE_FILE)
			{
				debug::printf((char*)"file: %s\n", dir.filename);
				LoadSectionItem(section->path, dir.filename, section->items->item[fileCount]);
				fileCount++;
			}
		}
		fsys::directoryClose(&d);
	}
	else
	{
		debug::printf("Failed to open folder: %s\n", section->path);
	}
	//Sort sections
	SortSectionItems(section->items, fileCount);
	section->items->count=fileCount;
	debug::printf((char*)"Final Item Count: %d\n", fileCount);

	//Arrange items on screen
	//link each item to the next item in each directiond
	int x=0, y=0;
	for(int i=0; i<section->items->count; i++)
	{
		SectionItem_t *item = section->items->item[i];

		item->x = x;
		item->y = y;

		x++;
		if(x>=SECTION_ITEMS_PER_ROW)
		{
			x=0;
			y++;
			if(y>=SECTION_ITEMS_PER_COL)
			{
				//We've run out of space - bail
				debug::printf("Too many items in section\n");
				break;
			}
		}
	}



	debug::printf("LoadSectionItems end\n");
	return true;
}

bool LoadSection(char *path, char *filename, Section_t *section)
{
	strcpy(section->filename, filename);
	GetDisplayName(section->filename,section->name);
	debug::printf((char*)"filename:%s\n",section->filename);
	debug::printf((char*)"name:%s\n",section->name);
	
	strcpy(section->path,path);
	fsys::combine(section->path, filename);
	debug::printf((char*)"path:%s\n",section->path);
	
	//Load section icon
	//Will be in /launcher/skins/<skin_name>/sections/<section_name>.png
	char iconFilename[HW_MAX_PATH];
	strcpy(iconFilename, mSkinPath);
	fsys::combine(iconFilename, SECTIONS_PATH);
	fsys::combine(iconFilename, section->name, "png");
	
	debug::printf((char*)"icon: %d path: %s\n", mIcon, iconFilename);
	
	int icon = LoadIcon(iconFilename, iconFilename);
	if(icon<0)
	{
		debug::printf((char*)"Failed to load png\n");
	}
	section->icon = icon; //record index of icon for section
	debug::printf("after load\n");
	
	if(!LoadSectionItems(section))
	{
		return false;
	}
}

bool LoadSections(char *path)
{
	//Get Number of directory in sections directory
	int dirCount=0;
	if(!fsys::directoryItemCountType(path, MENU_FILE_TYPE_DIRECTORY, &dirCount))
	{
		debug::printf((char*)"Failed to get count\n");
		return false;
	}
	
	debug::printf((char*)"Dir Count: %d\n", dirCount);
	int sectionMemSize = sizeof(SectionList)
	                   + (dirCount * sizeof(Section_t*))
	                   + (dirCount * sizeof(Section_t));

	if(mpSections != NULL)
	{
		//tear down sections in reverse order 
		for(int x=mpSections->count-1; x>=0; x--)
		{
			Section_t *section=mpSections->section[x];
			if(section->items != NULL)
			{
				free(section->items);
				section->items = NULL;
			}
		}
		free(mpSections);
		mpSection = NULL;
	}

	mpSections = (SectionList*)malloc(sectionMemSize);
	memset(mpSections,0,sectionMemSize);

	//Now setup pointers to each section item
	char *entryIdxMem=((char*)mpSections)+sizeof(SectionList);
	char *entryMem=(char*)entryIdxMem+(sizeof(Section_t*)*dirCount);

	mpSections->section = (Section_t**)entryIdxMem;
	for(int x=0;x<dirCount;x++)
	{
		mpSections->section[x]=(Section_t*)entryMem;
		entryMem+=sizeof(Section_t);
	}

	MENU_DIR d;
	dirCount=0;
	if (fsys::directoryOpen((char*)path, &d))
	{
		//Dir opened, now stream out details
		MENU_DIRECTORY_ENTRY dir;
		while(fsys::directoryRead(&d, &dir))
		{
			if(dir.type==MENU_FILE_TYPE_DIRECTORY)
			{
				if(menu::string_compare(dir.filename, "..") == 0 
				|| menu::string_compare(dir.filename, ".") == 0)
				{
					debug::printf((char*)"Ignoring non directory\n");
				}
				else
				{
					debug::printf((char*)"file: %s\n", dir.filename);
					LoadSection(path, dir.filename, mpSections->section[dirCount]);
					dirCount++;
				}
			}
		}
		fsys::directoryClose(&d);
	}
	debug::printf((char*)"here 2 2\n");
	//Sort sections
	SortSections(mpSections, dirCount);
	debug::printf((char*)"here 3 3\n");
	mpSections->count=dirCount;
	debug::printf((char*)"Final Section Count: %d\n", dirCount);
	return true;
}

void RenderSectionBar()
{
	//work out how much space between sections
	int sz = (HW_SCREEN_WIDTH / SECTION_ITEMS_PER_ROW);
	
	int offset = ((SECTION_ITEMS_PER_ROW - mpSections->count)*sz)>>1;
	for(int x=0; x<mpSections->count; x++)
	{
		Section_t *section=mpSections->section[x];
		if(section->icon >= 0)
		{
			video::draw_tex_transp((sz*x)+((sz-mpIcons[section->icon]->icon->width)>>1)+offset, 0, mpIcons[section->icon]->icon);
		}
		int tw,th;
		video::font_size(mpCurrFont, section->name, &th, &tw);
		video::print((sz*x+1)+(sz>>1)-(tw>>1)+offset, 32+1, section->name, COLOR_BLACK, mpCurrFont);
		video::print((sz*x)+(sz>>1)-(tw>>1)+offset, 32, section->name, COLOR_WHITE, mpCurrFont);
	}
}

void RenderSection(Section_t *section)
{
	
	if(mpSections->count > 1)
	{
		if(mpCurrTopBar != NULL)
		{
			video::draw_tex_transp(0, 0, mpCurrTopBar);
		}
		RenderSectionBar();
	}	
	
	int sx = (HW_SCREEN_WIDTH / SECTION_ITEMS_PER_ROW);
	int sy = ((HW_SCREEN_HEIGHT - mStartY) / SECTION_ITEMS_PER_COL);
	int x = 0, y = 0;

	for(int i=m1stDisplayedItem; i<section->items->count; i++)
	{
		SectionItem_t *item = section->items->item[i];
		int tw,th;
		video::font_size(mpCurrFont, item->name, &th, &tw);

		if(item->icon >= 0)
		{
			video::draw_tex_transp((sx*x)+((sx-mpIcons[item->icon]->icon->width)>>1), mStartY+(y*sy), mpIcons[item->icon]->icon);
		}

		
		video::print((sx*x+1)+(sx>>1)-(tw>>1), mStartY+(y*sy)+32+1, item->name, COLOR_BLACK, mpCurrFont);
		video::print((sx*x)+(sx>>1)-(tw>>1), mStartY+(y*sy)+32, item->name, COLOR_WHITE, mpCurrFont);
		x++;
		if(x>=SECTION_ITEMS_PER_ROW)
		{
			x=0;
			y++;
			if(y>=SECTION_ITEMS_PER_COL)
			{
				//We've run out of display space - bail
				return;
			}
		}
	}
}

void RenderSelection(int selectedSection)
{
	unsigned char rgba[]={255,255,255,130};
	int sz = (HW_SCREEN_WIDTH / SECTION_ITEMS_PER_ROW);
    int offset = ((SECTION_ITEMS_PER_ROW - mpSections->count)*sz)>>1;
	int x = (sz*selectedSection);
	int y = 0;
	int w = sz;

	//Alpha blend focus rectangle for mPosX and mPosY
	video::fill_rect(x+offset, y, w, 40, (unsigned char*)&rgba);

	x = sz*mPosX;
	y = mStartY+((HW_SCREEN_HEIGHT-mStartY) / SECTION_ITEMS_PER_COL)*mPosY;
	//Alpha blend focus rectangle for mPosX and mPosY
	video::fill_rect(x, y, w, 40, (unsigned char*)&rgba);
}

void Render(int selectedSection)
{
	video::draw_tex_transp(0, 0, mpCurrWallPaper);
	RenderSection(mpSection);
	RenderSelection(selectedSection);
}

void LoadFirstWallPaper()
{
	if(mpCurrWallPaper != NULL)
	{
		free(mpCurrWallPaper);
		mpCurrWallPaper = NULL;
	}

	MENU_DIR d;
	int fileCount=0;
	int fileMax=0;
	if (!fsys::directoryItemCountType(mWallpapersPath, MENU_FILE_TYPE_FILE, &fileCount))
	{
		debug::printf("Failed to read wallpaper root :%s\n", mWallpapersPath);
		return;
	}

	if(fileCount ==0) return;

	if (fsys::directoryOpen((char*)mWallpapersPath, &d))
	{
		//Dir opened, now stream out details
		MENU_DIRECTORY_ENTRY dir;
		while(fsys::directoryRead(&d, &dir))
		{
			if(dir.type==MENU_FILE_TYPE_FILE)
			{
				char path[HW_MAX_PATH];
				strcpy(path, mWallpapersPath);
				fsys::combine(path, dir.filename);
				strcpy(mWallpaperName, dir.filename);
				debug::printf((char*)"Wallpaper:%s\n",path);
				mpCurrWallPaper = video::tex_load(path);
				break;
			}
		}
		fsys::directoryClose(&d);
	}

	return;
}

int Launch(SectionItem_t *item)
{
	if(menu::string_compare(item->exec, "#exit")==0)
	{
		debug::printf("Exitr\n");
		return 1;
	}
	else if(menu::string_compare(item->exec, "#wallpaper")==0)
	{
		debug::printf("Do Wallpaper\n");
		launcher::menu_wallpaper();
		return 0;
	}
	else if(menu::string_compare(item->exec, "#skin")==0)
	{
		debug::printf("Do Skin\n");
		launcher::menu_skin();
		return 0;
	}

	if(system::chain_boot(item->exec))
	{
		return 1;
	}

	return 0;
}

void launcher::SetSkin(char *filename)
{
	//Icon cache will become invalid on skin change, so ditch it
	strcpy(mSkinName, filename);
	debug::printf("Free icon cached\n");
	if(mpIcons != NULL)
	{
		for(int x=mIcon-1;x>=0;x--)
		{
			if(mpIcons[x] != NULL)
			{
				if(mpIcons[x]->icon != NULL)
				{
					debug::printf("Free icon 1\n");
					free(mpIcons[x]->icon);
					mpIcons[x]->icon = NULL;
				}
				debug::printf("Free icon 2\n");
				free(mpIcons[x]);
				mpIcons[x]=NULL;
			}
		}
		debug::printf("Free icon 3\n");
		free(mpIcons);
	}

	debug::printf("Allocate icon cached\n");
	mpIcons = (Icon_t**)malloc(sizeof(Icon_t*)*mIconCount);
	memset(mpIcons, 0, sizeof(Icon_t*)*mIconCount);
	mIcon=0;

	strcpy(mSkinPath,mSkinsPath);
	fsys::combine(mSkinPath, filename);
	debug::printf((char*)"Skin Path:%s\n",mSkinPath);

	strcpy(mWallpapersPath, mSkinPath);
	fsys::combine(mWallpapersPath, WALLPAPER_PATH);
	debug::printf((char*)"Wallpaper Path:%s\n",mWallpapersPath);

	char topBarPath[HW_MAX_PATH];
	strcpy(topBarPath, mSkinPath);
	fsys::combine(topBarPath, IMGS_PATH);
	fsys::combine(topBarPath, "topbar.png");
	debug::printf((char*)"Topbar:%s\n",topBarPath);
	if(mpCurrTopBar!=NULL)
	{
		free(mpCurrTopBar);
		mpCurrTopBar=NULL;
	}
	mpCurrTopBar = video::tex_load(topBarPath);
	
	char fontPath[HW_MAX_PATH];
	strcpy(fontPath, mSkinPath);
	fsys::combine(fontPath, "font.ttf");
	debug::printf((char*)"Font:%s\n",fontPath);
	
	if(mpCurrFont!=NULL)
	{
		hw::video::font_delete(mpCurrFont);
		mpCurrFont=NULL;
	}

	debug::printf((char*)"Start TTF font\n");
	mpCurrFont = video::font_load(fontPath, 34, 10);
	debug::printf((char*)"End TTF font\n");

	if(mpCurrFont==NULL)
	{
		//Try to load default font
		debug::printf("Font load failed, trying default font\n");
		strcpy(fontPath, mSkinDefaultPath);
		fsys::combine(fontPath, "font.ttf");
		debug::printf((char*)"Font:%s\n",fontPath);
		mpCurrFont = video::font_load(fontPath, 34, 10);
	}

	LoadFirstWallPaper();

	video::font_set(mpCurrFont);

	debug::printf("LoadSections\n");
	if(!LoadSections(mSectionsPath))
	{
		debug::printf("LoadSections failed\n");
		return;
	}
	debug::printf("SetSkin Done\n");

	save_config();
}

bool load_config()
{
	bool ret=false;
	char filename[HW_MAX_PATH];
	char line[HW_MAX_PATH*2];
	char name[HW_MAX_PATH*2];
	char value[HW_MAX_PATH*2];

	strcpy(filename, fsys::getEmulPath());
	fsys::combine(filename, CONFIG_FILENAME);

	FILE *f = fopen(filename, "r");
	if(f == NULL)
	{
		debug::printf("Failed to open config file:%s\n", filename);
		return false;
	}
		

	while(fgets(line, HW_MAX_PATH*2, f)){
		debug::printf(line);
		//Need to split line by delimiter =
		if(!StringSplit(line,'=',name,value))
		{
			debug::printf("Invalid content in %s.  Content:%s\n", filename, line);
		}

		debug::printf("name:%s  value:%s\n", name, value);
		if(menu::string_compare(name,"skin")==0)
		{
			launcher::SetSkin(value);
			ret=true;
		}
		else if(menu::string_compare(name,"wallpaper")==0)
		{
			launcher::SetWallpaper(value);
		}
	}
     
	fclose(f);
	return ret;
}

void launcher::open()
{
	debug::printf("Here\n");
	strcpy(mSectionsPath, fsys::getEmulPath());
	fsys::combine(mSectionsPath, SECTIONS_PATH);
	debug::printf((char*)"Sections:%s\n",mSectionsPath);
	
	strcpy(mSkinDefaultPath, fsys::getEmulPath());
	fsys::combine(mSkinDefaultPath, SKINS_PATH);
	fsys::combine(mSkinDefaultPath, SKIN_NAME_DEFAULT);

	strcpy(mSkinsPath, fsys::getEmulPath());
	fsys::combine(mSkinsPath, SKINS_PATH);

	if(!load_config())
	{
		launcher::SetSkin(SKIN_NAME_DEFAULT);
	}

	if(mpSections->count > 1)
	{
		if(mpCurrTopBar != NULL)
		{
			mStartY = mpCurrTopBar->height;
		}
		else
		{
			mStartY = 50;
		}		
	}

	video::init(VM_FAST);
	int focus=0;
	MENU_ACTION action;
	int id=0x41;
	int timerLast = system::get_frames_count();
	int timer=0;
	int xpos=0, ypos=0;
	int currSection=0;
	mpSection=mpSections->section[currSection];
	while(1)
	{
		video::bind_framebuffer();
		Render(currSection);
		
		video::term_framebuffer();
		video::flip(0);
		while(1)
		{
			timer = system::get_frames_count();
			if(timerLast != timer)
			{
				timerLast=timer;
				break;
			}
		}

		
		uint32_t keysHeld   = input::poll();
		uint32_t keysRepeat = input::get_repeat();
		
		if (keysRepeat & HW_INPUT_A) {
			//Try to load selected item
			int selected = m1stDisplayedItem+(mPosY*SECTION_ITEMS_PER_ROW)+mPosX;
			if(selected < mpSection->items->count)
			{
				int action = Launch(mpSection->items->item[selected]);
				debug::printf("Return from launch\n");
				mpSection=mpSections->section[currSection];
				if(action) break;
			}
			
		}

		if (keysRepeat & HW_INPUT_R) {
			currSection++;
			if(currSection>=mpSections->count) currSection=0;
			mpSection=mpSections->section[currSection];
			mPosY=0;
			mPosX=0;
			ypos=0;
			m1stDisplayedItem=0;
		}
		
		if (keysRepeat & HW_INPUT_L) {
			currSection--;
			if(currSection<0) currSection=mpSections->count-1;
			mpSection=mpSections->section[currSection];
			mPosY=0;
			mPosX=0;
			ypos=0;
			m1stDisplayedItem=0;
		}

		if (keysRepeat & HW_INPUT_UP) {
			int maxRow = mpSection->items->count==0 ? 0 : mpSection->items->count-1;
			maxRow /= SECTION_ITEMS_PER_ROW;

			ypos--;
			if(ypos < 0 )
			{
				ypos = maxRow;

				if (ypos>=SECTION_ITEMS_PER_COL-1)
				{
					mPosY=SECTION_ITEMS_PER_COL-1;
				}
				else
				{
					mPosY=ypos;
				}
				debug::printf("mPosY:%d ypos:%d maxRow:%d\n", mPosY, ypos, maxRow);
				m1stDisplayedItem=((ypos-(mPosY))*SECTION_ITEMS_PER_ROW);
			}
			else
			{
				if(m1stDisplayedItem>0 && mPosY == 0)
				{
					m1stDisplayedItem-=SECTION_ITEMS_PER_ROW;
				}

				if (mPosY>0)
				{
					mPosY--;
				}
			}

			debug::printf("my:%d y:%d maxy:%d count:%d 1st:%d t:%d\n"
			 , mPosY, ypos, maxRow, mpSections->count, m1stDisplayedItem, ypos-(SECTION_ITEMS_PER_COL-mPosY));
		}

		if (keysRepeat & HW_INPUT_DOWN) { 
			
			int maxRow = mpSection->items->count==0 ? 0 : mpSection->items->count-1;
			maxRow /= SECTION_ITEMS_PER_ROW;

			

			ypos++;
			if(ypos <= maxRow) 
			{
				if(ypos>=SECTION_ITEMS_PER_COL && mPosY==SECTION_ITEMS_PER_COL-1)
				{
					m1stDisplayedItem+=SECTION_ITEMS_PER_ROW;
				}

				if (mPosY<SECTION_ITEMS_PER_COL-1)
				{
					mPosY++;
				}
			}
			else
			{
				ypos=0;
				mPosY=0;
				m1stDisplayedItem=0;
			}
			

			debug::printf("my:%d y:%d maxy:%d count:%d 1st:%d t:%d\n"
			 , mPosY, ypos, maxRow, mpSections->count, m1stDisplayedItem, ypos-(SECTION_ITEMS_PER_COL-mPosY));
		}

		if (keysRepeat & HW_INPUT_LEFT) {
			mPosX--;
			if (mPosX<0) mPosX=SECTION_ITEMS_PER_ROW-1;
		}

		if (keysRepeat & HW_INPUT_RIGHT) {
			mPosX++;
			if (mPosX>=SECTION_ITEMS_PER_ROW) mPosX=0;
		}

		if (keysHeld & HW_INPUT_SELECT) {
			break;
		}
	}

	//Handle input
	
	//free(mpSections);
}



void launcher_close()
{
	video::font_delete(video::font_get());
}



