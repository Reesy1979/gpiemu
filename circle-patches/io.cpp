#include "config.h"

#include "wrap_fatfs.h"

#include <_ansi.h>
#include <_syslist.h>
#include <sys/types.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <circle/input/console.h>
#include <circle/sched/scheduler.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/string.h>
#include "circle_glue.h"
#include <assert.h>
#include <stdarg.h>

struct _CIRCLE_DIR
{
    _CIRCLE_DIR ()
        :
        mFirstRead (0), mOpen (0)
    {
        mEntry.d_ino = 0;
        mEntry.d_name[0] = 0;
    }

    FATFS_DIR mCurrentEntry;
    struct dirent mEntry;
    unsigned int mFirstRead :1;
    unsigned int mOpen :1;
};

namespace
{
    constexpr unsigned int MAX_OPEN_FILES = 20;
    constexpr unsigned int MAX_OPEN_DIRS = 20;

    class CGlueIO;
    struct CircleFile
    {
        CircleFile ()
            :
            mCGlueIO (nullptr)
        {
        }
        CGlueIO *mCGlueIO;
    };

    CircleFile fileTab[MAX_OPEN_FILES];
    CSpinLock fileTabLock(TASK_LEVEL);

    _CIRCLE_DIR dirTab[MAX_OPEN_DIRS];
    CSpinLock dirTabLock(TASK_LEVEL);

    /**
     * Helper class to acquire lock and to release it automatically
     * when surrounding block is left.
     */
    class SpinLockHolder
    {
    public:
        SpinLockHolder (CSpinLock &lock) : lockRef(lock)
        {
            lockRef.Acquire ();
        }

        ~SpinLockHolder ()
        {
            lockRef.Release ();
        }

    private:
        CSpinLock &lockRef;
    };

    class CGlueIO
    {
    public:
        CGlueIO() : mRefCount(1)
        {
        }

        virtual
        ~CGlueIO ()
        {
        }

        virtual int
        Read (void *pBuffer, int nCount) = 0;

        virtual int
        Write (const void *pBuffer, int nCount) = 0;

        virtual int
        LSeek(int ptr, int dir) = 0;

        virtual int
        Close (void) = 0;

        virtual int
        FTruncate (off_t)
        {
            errno = EINVAL;
            return -1;
        }

        virtual int
        FSync (void)
        {
            errno = EINVAL;
            return -1;
        }

        virtual int
        FStat (struct stat *buf) = 0;

        virtual int
        IsATty (void) = 0;

        void IncrementRefCount (void)
        {
            mRefCount += 1;
        }

        void DecrementRefCount (void)
        {
            assert (mRefCount > 0);
            mRefCount -= 1;
        }

        unsigned int GetRefCount (void) const
        {
            return mRefCount;
        }

    private:
        unsigned int mRefCount;
    };

    class CGlueConsole : public CGlueIO
    {
    public:
        enum TConsoleMode
        {
            ConsoleModeRead, ConsoleModeWrite
        };

        CGlueConsole (CConsole &rConsole, TConsoleMode mode)
            :
            mConsole (rConsole), mMode (mode)
        {
        }

        int
        Read (void *pBuffer, int nCount)
        {
            int nResult = -1;

            if (mMode == ConsoleModeRead)
            {
                CScheduler * const scheduler =
                    CScheduler::IsActive () ? CScheduler::Get () : nullptr;
                CUSBHostController * const usbhost =
                    CUSBHostController::IsActive () ?
                        CUSBHostController::Get () : nullptr;

                while ((nResult = mConsole.Read (pBuffer,
                                                 static_cast<size_t> (nCount)))
                    == 0)
                {
                    if (scheduler)
                    {
                        scheduler->Yield ();
                    }

                    if (usbhost && usbhost->UpdatePlugAndPlay ())
                    {
                        mConsole.UpdatePlugAndPlay ();
                    }
                }
            }

            if (nResult < 0)
            {
                errno = EIO;

                // Could be negative but different from -1.
                nResult = -1;
            }

            return nResult;
        }

        int
        Write (const void *pBuffer, int nCount)
        {
            int nResult =
                mMode == ConsoleModeWrite ?
                    mConsole.Write (pBuffer, static_cast<size_t> (nCount)) : -1;

            if (nResult < 0)
            {
                errno = EIO;

                // Could be negative but different from -1.
                nResult = -1;
            }

            return nResult;
        }

        int
        LSeek(int ptr, int dir)
        {
            errno = ESPIPE;
            return -1;
        }

        int
        Close (void)
        {
            // TODO: Cannot close console file handle currently.
            errno = EBADF;
            return -1;
        }

        int
        FStat (struct stat *buf)
        {
            errno = EBADF;
            return -1;
        }

        int
        IsATty (void)
        {
            return 1;
        }

    private:
        CConsole &mConsole;
        TConsoleMode const mMode;
    };

    /**
     * FatFs: http://www.elm-chan.org/fsw/ff/00index_e.html
     */
    struct CGlueIoFatFs : public CGlueIO
    {
        CGlueIoFatFs ()
            : mFilename (nullptr)
        {
            memset (&mFile, 0, sizeof(mFile));
        }

        ~CGlueIoFatFs ()
        {
            free (mFilename);
        }

        bool
        Open (char *file, int flags, int /* mode */)
        {
            // Remember file name for simulation of fstat ()
            mFilename = (char *) malloc (strlen (file) + 1);
            if (!mFilename)
            {
                errno = ENOMEM;
                return false;
            }
            strcpy(mFilename, file);

            int fatfs_flags;

            /*
             * The OpenGroup documentation of the flags for open() says:
             *
             * "Applications shall specify exactly one of the first
             * three values (file access modes) below in the value of oflag:
             *
             * O_RDONLY Open for reading only.
             * O_WRONLY Open for writing only.
             * O_RDWR Open for reading and writing."
             *
             * So combinations of those flags need not to be dealt with.
             */
            if (flags & O_RDWR)
            {
                fatfs_flags = FA_READ | FA_WRITE;
            }
            else if (flags & O_WRONLY)
            {
                fatfs_flags = FA_WRITE;
            }
            else
            {
                fatfs_flags = FA_READ;
            }

            if (flags & O_TRUNC)
            {
                /*
                 * OpenGroup documentation:
                 * "The result of using O_TRUNC with O_RDONLY is undefined."
                 */
                fatfs_flags |= FA_CREATE_ALWAYS;
            }

            if (flags & O_APPEND)
            {
                fatfs_flags |= FA_OPEN_APPEND;
            }

            if (flags & O_CREAT)
            {
                if (flags & O_EXCL)
                {
                    /*
                     * OpenGroup documentation:
                     * "O_EXCL If O_CREAT and O_EXCL are set, open() shall fail if the file exists.
                     * "If O_EXCL is set and O_CREAT is not set, the result is undefined.".
                     */
                    fatfs_flags |= FA_CREATE_NEW;
                }
                else
                {
                    fatfs_flags |= FA_OPEN_ALWAYS;
                }
            }

            // TODO mode?
            auto const fresult = f_open (&mFile, file, fatfs_flags);

            bool result;
            if (fresult == FR_OK)
            {
                result = true;
            }
            else
            {
                result = false;

                switch (fresult)
                {
                    case FR_EXIST:
                        errno = EEXIST;
                        break;

                    case FR_NO_FILE:
                    case FR_INVALID_NAME:
                        errno = ENOENT;
                        break;

                    case FR_NO_PATH:
                    case FR_INVALID_DRIVE:
                        errno = ENOTDIR;
                        break;

                    case FR_NOT_ENOUGH_CORE:
                        errno = ENOMEM;
                        break;

                    case FR_TOO_MANY_OPEN_FILES:
                        errno = ENFILE;
                        break;

                    case FR_DENIED:
                    case FR_DISK_ERR:
                    case FR_INT_ERR:
                    case FR_NOT_READY:
                    case FR_INVALID_OBJECT:
                    case FR_WRITE_PROTECTED:
                    case FR_NOT_ENABLED:
                    case FR_NO_FILESYSTEM:
                    case FR_TIMEOUT:
                    case FR_LOCKED:
                    default:
                        errno = EACCES;
                        break;
                }
            }

            return result;
        }

        int
        Read (void *pBuffer, int nCount)
        {
            UINT bytes_read = 0;
            auto const fresult = f_read (&mFile, pBuffer,
                                         static_cast<UINT> (nCount),
                                         &bytes_read);

            int result;
            if (fresult == FR_OK)
            {
                result = static_cast<int> (bytes_read);
            }
            else
            {
                result = -1;

                switch (fresult)
                {
                    case FR_INVALID_OBJECT:
                    case FR_DENIED:
                        errno = EBADF;
                        break;

                    case FR_DISK_ERR:
                    case FR_INT_ERR:
                    case FR_TIMEOUT:
                    default:
                        errno = EIO;
                        break;
                }
            }

            return result;
        }

        int
        Write (const void *pBuffer, int nCount)
        {
            UINT bytesWritten = 0;
            auto const fresult = f_write (&mFile, pBuffer,
                                          static_cast<UINT> (nCount),
                                          &bytesWritten);

            int result;
            if (fresult == FR_OK)
            {
                result = static_cast<int> (bytesWritten);
                f_sync(&mFile);
            }
            else
            {
                result = -1;

                switch (fresult)
                {
                    case FR_INVALID_OBJECT:
                        errno = EBADF;
                        break;

                    case FR_DISK_ERR:
                    case FR_INT_ERR:
                    case FR_DENIED:
                    case FR_TIMEOUT:
                    default:
                        errno = EIO;
                        break;
                }
            }

            return result;
        }

        int
        LSeek(int ptr, int dir)
        {
            /*
             * If whence is SEEK_SET, the file offset shall be set to
             * offset bytes.
             *
             * If whence is SEEK_CUR, the file offset shall be set to
             * its current location plus offset.
             *
             * If whence is SEEK_END, the file offset shall be set to
             * the size of the file plus offset.
             */
            int new_pos;
            switch (dir)
            {
                case SEEK_SET:
                    new_pos = ptr;
                    break;

                case SEEK_CUR:
                    new_pos = static_cast<int>(f_tell(&mFile)) + ptr;
                    break;

                case SEEK_END:
                    new_pos = static_cast<int>(f_size(&mFile)) + ptr;
                    break;

                default:
                    new_pos = -1;
                    break;
            }

            int result;
            if (new_pos >= 0)
            {
                int const fresult = f_lseek(&mFile, static_cast<FSIZE_t>(new_pos));

                switch (fresult)
                {
                    case FR_OK:
                        result = new_pos;
                        break;

                    case FR_DISK_ERR:
                    case FR_INT_ERR:
                    case FR_INVALID_OBJECT:
                    case FR_TIMEOUT:
                    default:
                        result = -1;
                        errno = EBADF;
                        break;
                }
            }
            else
            {
                errno = EINVAL;
                result = -1;
            }

            return result;
        }

        int
        Close (void)
        {
            auto const close_result = f_close (&mFile);

            int result;
            switch (close_result)
            {
                case FR_OK:
                    result = 0;
                    break;

                case FR_INVALID_OBJECT:
                case FR_INT_ERR:
                    errno = EBADF;
                    result = -1;
                    break;

                default:
                    errno = EIO;
                    result = -1;
                    break;
            }

            if (mFilename)
            {
                free (mFilename);
                mFilename = nullptr;
            }

            return result;
        }

        virtual int
        FTruncate (off_t length)
        {
            if (length < 0)
            {
                errno = EINVAL;
                return -1;
            }

            FSIZE_t const cur_pos = f_tell(&mFile);

            if (f_lseek(&mFile, (FSIZE_t) length) != F_OK)
            {
                errno = EIO;
                return -1;
            }

            FRESULT const fresult = f_truncate(&mFile);
            int result;
            if (fresult == F_OK)
            {
                result = 0;
            }
            else
            {
                result = -1;

                // Best effort to map error codes:
                switch (fresult)
                {
                case FR_DENIED:
                    errno = EINVAL;
                    break;

                default:
                    errno = EIO;
                    break;
                }
            }

            // Best effort to restore seek position even if error occurred.
            f_lseek(&mFile, cur_pos);

            return result;
        }

        int
        FSync (void)
        {
            int const result = f_sync (&mFile) == FR_OK ? 0 : -1;

            if (result != 0)
            {
                errno = EIO;
            }

            return result;
        }

        int
        FStat (struct stat *buf)
        {
            // We need to force an fsync() because the fstat is simulated
            // via a stat() call using the saved filename.
            if (FSync () == -1)
            {
                return -1;
            }

            assert (mFilename);
            return stat (mFilename, buf);
        }

        int
        IsATty (void)
        {
            errno = ENOTTY;
            return 0;
        }

        FIL mFile;
        char *mFilename;
    };

    int
    FindFreeFileSlot (unsigned int start_index = 0)
    {
        int slotNr = -1;

        for (unsigned int i = start_index; i < MAX_OPEN_FILES; i += 1)
        {
            if (fileTab[i].mCGlueIO == nullptr)
            {
                slotNr = static_cast<int>(i);
                break;
            }
        }

        return slotNr;
    }

    int
    FindFreeDirSlot (void)
    {
        int slotNr = -1;

        for (auto const &slot : dirTab)
        {
            if (!slot.mOpen)
            {
                slotNr = &slot - dirTab;
                break;
            }
        }

        return slotNr;
    }

    void
    CGlueInitConsole (CConsole &rConsole)
    {
        CircleFile &circle_stdin = fileTab[0];
        CircleFile &circle_stdout = fileTab[1];
        CircleFile &circle_stderr = fileTab[2];

        // Must only be called once and not be called after a file has already been opened
        assert(!circle_stdin.mCGlueIO);
        assert(!circle_stdout.mCGlueIO);
        assert(!circle_stderr.mCGlueIO);

        circle_stdin.mCGlueIO = new CGlueConsole (rConsole,
                                           CGlueConsole::ConsoleModeRead);
        circle_stdout.mCGlueIO = new CGlueConsole (rConsole,
                                            CGlueConsole::ConsoleModeWrite);
        circle_stderr.mCGlueIO = new CGlueConsole (rConsole,
                                            CGlueConsole::ConsoleModeWrite);
    }
}

void
CGlueStdioInit (CConsole &rConsole)
{
    CGlueInitConsole (rConsole);
}

extern "C" int
_open (char *file, int flags, int mode)
{
    SpinLockHolder const lockHolder(fileTabLock);

    int slot = FindFreeFileSlot ();

    if (slot != -1)
    {
        auto const newFatFs = new CGlueIoFatFs ();

        if (newFatFs->Open (file, flags, mode))
        {
            fileTab[slot].mCGlueIO = newFatFs;
        }
        else
        {
            delete newFatFs;
            slot = -1;
        }
    }
    else
    {
        errno = ENFILE;
    }

    return slot;
}

extern "C" int
_close (int fildes)
{
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    SpinLockHolder const lockHolder(fileTabLock);

    CircleFile &file = fileTab[fildes];
    if (file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    assert (file.mCGlueIO->GetRefCount () > 0);
    file.mCGlueIO->DecrementRefCount ();

    int result;
    if (file.mCGlueIO->GetRefCount () == 0)
    {
        result = file.mCGlueIO->Close ();
        delete file.mCGlueIO;
    }
    else
    {
        result = 0;
    }

    file.mCGlueIO = nullptr;

    return result;
}

extern "C" int
_read (int fildes, char *ptr, int len)
{
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    CircleFile &file = fileTab[fildes];
    if (file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return file.mCGlueIO->Read (ptr, len);
}

extern "C" int
_write (int fildes, char *ptr, int len)
{
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    CircleFile &file = fileTab[fildes];
    if (file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return file.mCGlueIO->Write (ptr, len);
}

extern "C" int
_lseek(int fildes, int ptr, int dir)
{
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    CircleFile &file = fileTab[fildes];
    if (file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return file.mCGlueIO->LSeek (ptr, dir);
}

extern "C" int
_fstat (int fildes, struct stat *st)
{
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    CircleFile &file = fileTab[fildes];
    if (file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return file.mCGlueIO->FStat (st);
}

extern "C" int
ftruncate (int fildes, off_t length)
{
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    CircleFile &file = fileTab[fildes];
    if (file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return file.mCGlueIO->FTruncate (length);
}

template<int (CGlueIO::*func) (void)>
int call_glueio_func_void_arg_valid_fildes(int fildes)
{
    CircleFile &file = fileTab[fildes];

    if (file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return (file.mCGlueIO->*func) ();
}

template<int (CGlueIO::*func) (void)>
int call_glueio_func_void_arg(int fildes)
{
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    return call_glueio_func_void_arg_valid_fildes<func> (fildes);
}

extern "C" int
fsync (int fildes)
{
    return call_glueio_func_void_arg<&CGlueIO::FSync> (fildes);
}

extern "C" int
_isatty (int fildes)
{
    return call_glueio_func_void_arg<&CGlueIO::IsATty> (fildes);
}

extern "C" DIR*
opendir (const char *name)
{
    SpinLockHolder const lockHolder(dirTabLock);

    int const slotNum = FindFreeDirSlot ();
    if (slotNum == -1)
    {
        errno = ENFILE;
        return nullptr;
    }

    auto &slot = dirTab[slotNum];

    FRESULT const fresult = f_opendir (&slot.mCurrentEntry, name);

    /*
     * Best-effort mapping of FatFs error codes to errno values.
     */
    DIR *result = nullptr;
    switch (fresult)
    {
        case FR_OK:
            slot.mOpen = 1;
            slot.mFirstRead = 1;
            result = &slot;
            break;

        case FR_DISK_ERR:
        case FR_INT_ERR:
        case FR_NOT_READY:
        case FR_INVALID_OBJECT:
            errno = EACCES;
            break;

        case FR_NO_PATH:
        case FR_INVALID_NAME:
        case FR_INVALID_DRIVE:
        case FR_NOT_ENABLED:
        case FR_NO_FILESYSTEM:
        case FR_TIMEOUT:
            errno = ENOENT;
            break;

        case FR_NOT_ENOUGH_CORE:
        case FR_TOO_MANY_OPEN_FILES:
            errno = ENFILE;
            break;
    }

    return result;
}

static bool
read_next_entry (FATFS_DIR *currentEntry, FILINFO *filinfo)
{
    bool result;

    if (f_readdir (currentEntry, filinfo) == FR_OK)
    {
        result = filinfo->fname[0] != 0;
    }
    else
    {
        errno = EBADF;
        result = false;
    }

    return result;
}

static struct dirent*
do_readdir (DIR *dir, struct dirent *de)
{
    FILINFO filinfo;
    bool haveEntry;

    if (dir->mFirstRead)
    {
        if (f_readdir (&dir->mCurrentEntry, nullptr) == FR_OK)
        {
            haveEntry = read_next_entry (&dir->mCurrentEntry, &filinfo);
        }
        else
        {
            errno = EBADF;
            haveEntry = false;
        }
        dir->mFirstRead = 0;
    }
    else
    {
        haveEntry = read_next_entry (&dir->mCurrentEntry, &filinfo);
    }

    struct dirent *result;
    if (haveEntry)
    {
        memcpy (de->d_name, filinfo.fname, sizeof(de->d_name));
        de->d_ino = 0; // inode number does not exist in fatfs
        de->d_type = filinfo.fattrib;
        result = de;
    }
    else
    {
        // end of directory does not change errno
        result = nullptr;
    }

    return result;
}

extern "C" struct dirent*
readdir (DIR *dir)
{
    struct dirent *result;

    if (dir->mOpen)
    {
        result = do_readdir (dir, &dir->mEntry);
    }
    else
    {
        errno = EBADF;
        result = nullptr;
    }

    return result;
}

extern "C" int
readdir_r (DIR *__restrict dir, dirent *__restrict de, dirent **__restrict ode)
{
    int result;

    if (dir->mOpen)
    {
        *ode = do_readdir (dir, de);
        result = 0;
    }
    else
    {
        *ode = nullptr;
        result = EBADF;
    }

    return result;
}

extern "C" void
rewinddir (DIR *dir)
{
    dir->mFirstRead = 1;
}

extern "C" int
closedir (DIR *dir)
{
    SpinLockHolder const lockHolder(dirTabLock);

    int result;

    if (dir->mOpen)
    {
        dir->mOpen = 0;

        if (f_closedir (&dir->mCurrentEntry) == FR_OK)
        {
            result = 0;
        }
        else
        {
            errno = EBADF;
            result = -1;
        }
    }
    else
    {
        errno = EBADF;
        result = -1;
    }

    return result;
}

namespace {
    int
    dupfd (CircleFile &original_file, int start_slot)
    {
        if (start_slot < 0 || start_slot >= MAX_OPEN_FILES)
        {
            errno = EINVAL;
            return -1;
        }

        int const result =
            FindFreeFileSlot(static_cast<unsigned int>(start_slot));

        if (result != -1)
        {
            CircleFile &new_file = fileTab[result];

            assert (new_file.mCGlueIO == nullptr);
            new_file.mCGlueIO = original_file.mCGlueIO;
            new_file.mCGlueIO->IncrementRefCount();
        }
        else
        {
            errno = EMFILE;
        }

        return result;
    }
}

extern "C" int
fcntl (int fildes, int cmd, ...)
{
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    CircleFile &original_file = fileTab[fildes];

    SpinLockHolder const lockHolder(fileTabLock);

    if (original_file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    if (cmd != F_DUPFD)
    {
        errno = ENOSYS;
        return -1;
    }

    va_list args;
    va_start(args, cmd);
    int const arg = va_arg(args, int);
    va_end(args);

    // TODO: F_DUPFD is the only operation implemented so far.
	return dupfd (original_file, arg);
}

extern "C" int
dup(int fildes)
{
    return fcntl (fildes, F_DUPFD, 0);
}

extern "C" int
dup2 (int fildes, int fildes2)
{
    // From the OpenGroup specification:
    // "If fildes2 is less than 0 or greater than or equal to {OPEN_MAX},
    // dup2() shall return -1 with errno set to [EBADF]."
    if (fildes < 0 || static_cast<unsigned int> (fildes) >= MAX_OPEN_FILES)
    {
        errno = EBADF;
        return -1;
    }

    CircleFile &original_file = fileTab[fildes];

    SpinLockHolder const lockHolder(fileTabLock);

    if (original_file.mCGlueIO == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    // From the OpenGroup specification:
    // "If fildes is a valid file descriptor and is equal to fildes2,
    // dup2() shall return fildes2 without closing it."
    if (fildes == fildes2)
    {
        return fildes2;
    }

    _close (fildes2);
    int const result = dupfd (original_file, fildes2);

    assert (result == -1 || result == fildes2);

    return result;
}
