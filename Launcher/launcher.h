
#ifndef _LAUNCHER_H_
#define _LAUNCHER_H_

namespace launcher
{

    void open();
    char *GetSkinsPath();
    char *GetDefaultSkinPath();
    char *GetWallpapersPath();
    void SetWallpaper(char *filename);
    void SetSkin(char *filename);
    void menu_wallpaper();
    void menu_skin();
};

#endif //_LAUNCHER_H_
