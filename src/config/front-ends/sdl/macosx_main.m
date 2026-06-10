/*   Simplified SDL2 macOS entry point for Executor.
     Originally based on SDLMain.m by Darrell Walisser and Max Horn.
     CPS dock-registration hacks removed (obsolete since macOS 10.7).
*/

#define SDL_MAIN_HANDLED
#import <SDL2/SDL.h>

#import <Cocoa/Cocoa.h>
#import <sys/param.h>
#import <unistd.h>

static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;

static NSString *getApplicationName(void)
{
    NSDictionary *dict;
    NSString *appName = nil;

    dict = (NSDictionary *)CFBundleGetInfoDictionary(CFBundleGetMainBundle());
    if (dict)
        appName = [dict objectForKey: @"CFBundleName"];

    if (![appName length])
        appName = [[NSProcessInfo processInfo] processName];

    return appName;
}

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
- (void)terminate:(id)sender
{
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}
@end

@interface SDLMain : NSObject <NSApplicationDelegate>
@end

@implementation SDLMain

- (void)setupWorkingDirectory:(BOOL)shouldChdir
{
    if (shouldChdir) {
        char parentdir[MAXPATHLEN];
        CFURLRef url  = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
        if (CFURLGetFileSystemRepresentation(url2, true,
                                             (UInt8 *)parentdir, MAXPATHLEN))
            assert(chdir(parentdir) == 0);
        CFRelease(url);
        CFRelease(url2);
    }
}

static void setApplicationMenu(void)
{
    NSString    *appName = getApplicationName();
    NSMenu      *appleMenu;
    NSMenuItem  *menuItem;
    NSString    *title;

    appleMenu = [[NSMenu alloc] initWithTitle:@""];

    title = [@"About " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title
                         action:@selector(orderFrontStandardAboutPanel:)
                  keyEquivalent:@""];
    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Hide " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

    menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Hide Others"
                                                  action:@selector(hideOtherApplications:)
                                           keyEquivalent:@"h"];
    [menuItem setKeyEquivalentModifierMask:
        (NSEventModifierFlagOption | NSEventModifierFlagCommand)];

    [appleMenu addItemWithTitle:@"Show All"
                         action:@selector(unhideAllApplications:)
                  keyEquivalent:@""];
    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Quit " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

    menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:appleMenu];
    [[NSApp mainMenu] addItem:menuItem];

    [appleMenu release];
    [menuItem release];
}

static void setupWindowMenu(void)
{
    NSMenu     *windowMenu;
    NSMenuItem *windowMenuItem;
    NSMenuItem *menuItem;

    windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

    menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize"
                                          action:@selector(performMiniaturize:)
                                   keyEquivalent:@"m"];
    [windowMenu addItem:menuItem];
    [menuItem release];

    windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window"
                                                action:nil
                                         keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:windowMenuItem];
    [NSApp setWindowsMenu:windowMenu];

    [windowMenu release];
    [windowMenuItem release];
}

static void CustomApplicationMain(int argc, char **argv)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    SDLMain *sdlMain;

    [SDLApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp setMainMenu:[[NSMenu alloc] init]];
    setApplicationMenu();
    setupWindowMenu();

    sdlMain = [[SDLMain alloc] init];
    [NSApp setDelegate:sdlMain];

    [NSApp run];

    [sdlMain release];
    [pool release];
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    const char *temparg;
    size_t arglen;
    char *arg;
    char **newargv;

    if (!gFinderLaunch || gCalledAppMainline)
        return FALSE;

    temparg = [filename UTF8String];
    arglen  = SDL_strlen(temparg) + 1;
    arg     = (char *)SDL_malloc(arglen);
    if (!arg) return FALSE;

    newargv = (char **)realloc(gArgv, sizeof(char *) * (gArgc + 2));
    if (!newargv) { SDL_free(arg); return FALSE; }
    gArgv = newargv;

    SDL_strlcpy(arg, temparg, arglen);
    gArgv[gArgc++] = arg;
    gArgv[gArgc]   = NULL;
    return TRUE;
}

- (void)applicationDidFinishLaunching:(NSNotification *)note
{
    int status;

    [self setupWorkingDirectory:gFinderLaunch];

    gCalledAppMainline = TRUE;
    SDL_SetMainReady();
    status = SDL_main(gArgc, gArgv);
    exit(status);
}
@end


#ifdef main
#  undef main
#endif

int main(int argc, char **argv)
{
    if (argc >= 2 && strncmp(argv[1], "-psn", 4) == 0) {
        gArgv    = (char **)SDL_malloc(sizeof(char *) * 2);
        gArgv[0] = argv[0];
        gArgv[1] = NULL;
        gArgc    = 1;
        gFinderLaunch = YES;
    } else {
        int i;
        gArgc = argc;
        gArgv = (char **)SDL_malloc(sizeof(char *) * (argc + 1));
        for (i = 0; i <= argc; i++)
            gArgv[i] = argv[i];
        gFinderLaunch = NO;
    }

    CustomApplicationMain(argc, argv);
    return 0;
}
