//
//  HUDApp.mm
//  TrollSpeed
//
//  Created by Lessica on 2024/1/24.
//

#import <notify.h>

// rootless.h content
#include <sys/syslimits.h>
#include <unistd.h>

#ifdef XINA_SUPPORT
#define ROOT_PATH(cPath) !access("/var/LIY", F_OK) ? "/var/jb" cPath : cPath
#define ROOT_PATH_NS(path) !access("/var/LIY", F_OK) ? @"/var/jb" path : path
#define ROOT_PATH_NS_VAR !access("/var/LIY", F_OK) ? [@"/var/jb" stringByAppendingPathComponent:path] : path
#define ROOT_PATH_VAR(path) !access("/var/LIY", F_OK) ? ({ \
	char outPath[PATH_MAX]; \
	strlcpy(outPath, "/var/jb", PATH_MAX); \
	strlcat(outPath, path, PATH_MAX); \
	outPath; \
}) : path
#else
#define ROOT_PATH(cPath) THEOS_PACKAGE_INSTALL_PREFIX cPath
#define ROOT_PATH_NS(path) @THEOS_PACKAGE_INSTALL_PREFIX path
#define ROOT_PATH_NS_VAR(path) [@THEOS_PACKAGE_INSTALL_PREFIX stringByAppendingPathComponent:path]
#define ROOT_PATH_VAR(path) sizeof(THEOS_PACKAGE_INSTALL_PREFIX) > 1 ? ({ \
    char outPath[PATH_MAX]; \
    strlcpy(outPath, THEOS_PACKAGE_INSTALL_PREFIX, PATH_MAX); \
	strlcat(outPath, path, PATH_MAX); \
    outPath; \
}) : path
#endif

#import <mach-o/dyld.h>
#import <sys/utsname.h>
#import <objc/runtime.h>

#import "HUDHelper.h"
#import "TSEventFetcher.h"

// BackboardServices.h content
#import <Foundation/Foundation.h>

// IOKit+SPI.h content
typedef struct __IOHIDEvent *IOHIDEventRef;
typedef struct __IOHIDNotification *IOHIDNotificationRef;
typedef struct __IOHIDService *IOHIDServiceRef;
typedef struct __GSEvent *GSEventRef;

#if __cplusplus
extern "C" {
#endif
void *BKSHIDEventRegisterEventCallback(void (*)(void *, void *, IOHIDServiceRef, IOHIDEventRef));
void UIApplicationInstantiateSingleton(id aclass);
void UIApplicationInitialize();
void BKSDisplayServicesStart();
void GSInitialize();
void GSEventInitialize(Boolean registerPurple);
void GSEventPopRunLoopMode(CFStringRef mode);
void GSEventPushRunLoopMode(CFStringRef mode);
void GSEventRegisterEventCallBack(void (*)(GSEventRef));
#if __cplusplus
}
#endif

// AXEventPathInfoRepresentation.h content
@interface AXEventPathInfoRepresentation : NSObject
@property (assign, nonatomic) unsigned char pathIdentity;
@end

// AXEventHandInfoRepresentation.h content
@interface AXEventHandInfoRepresentation : NSObject
- (NSArray <AXEventPathInfoRepresentation *> *)paths;
@end

// AXEventRepresentation.h content
@interface AXEventRepresentation : NSObject
@property (nonatomic, readonly) BOOL isTouchDown; 
@property (nonatomic, readonly) BOOL isMove; 
@property (nonatomic, readonly) BOOL isChordChange; 
@property (nonatomic, readonly) BOOL isLift; 
@property (nonatomic, readonly) BOOL isInRange; 
@property (nonatomic, readonly) BOOL isInRangeLift; 
@property (nonatomic, readonly) BOOL isCancel; 
+ (instancetype)representationWithHIDEvent:(IOHIDEventRef)event hidStreamIdentifier:(NSString *)identifier;
- (AXEventHandInfoRepresentation *)handInfo;
- (CGPoint)location;
@end

// UIApplication+Private.h content
#import <UIKit/UIKit.h>

@interface UIApplication (Private)
- (UIEvent *)_touchesEvent;
- (void)_run;
- (void)suspend;
- (void)_accessibilityInit;
- (void)terminateWithSuccess;
- (void)__completeAndRunAsPlugin;
- (id)_systemAnimationFenceExemptQueue;
- (void)_enqueueHIDEvent:(IOHIDEventRef)event;
@end

NSString *mDeviceModel(void) {
    struct utsname systemInfo;
    uname(&systemInfo);
    return [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
}

void _HUDEventCallback(void *target, void *refcon, IOHIDServiceRef service, IOHIDEventRef event)
{
    static UIApplication *app = [UIApplication sharedApplication];
    
    // iOS 15.1+ has a new API for handling HID events.
    if (@available(iOS 15.1, *)) {}
    else {
        [app _enqueueHIDEvent:event];
    }

    BOOL shouldUseAXEvent = YES;  // Always use AX events now...

    BOOL isExactly15 = NO;
    static NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    if (version.majorVersion == 15 && version.minorVersion == 0 && version.patchVersion == 0) {
        NSString *deviceModel = mDeviceModel();
        if (![deviceModel hasPrefix:@"iPhone13,"] && ![deviceModel hasPrefix:@"iPhone14,"]) { // iPhone 12 & 13 Series
            isExactly15 = YES;
        }
    }

    if (@available(iOS 15.0, *)) {
        shouldUseAXEvent = !isExactly15;
    } else {
        shouldUseAXEvent = NO;
    }

    if (shouldUseAXEvent)
    {
        static Class AXEventRepresentationCls = nil;
        static dispatch_once_t onceToken;
        dispatch_once(&onceToken, ^{
            [[NSBundle bundleWithPath:@"/System/Library/PrivateFrameworks/AccessibilityUtilities.framework"] load];
            AXEventRepresentationCls = objc_getClass("AXEventRepresentation");
        });

        AXEventRepresentation *rep = [AXEventRepresentationCls representationWithHIDEvent:event hidStreamIdentifier:@"UIApplicationEvents"];

        /* I don't like this. It's too hacky, but it works. */
        {
            dispatch_async(dispatch_get_main_queue(), ^(void) {
                static UIWindow *keyWindow = nil;
                static dispatch_once_t onceToken;
                dispatch_once(&onceToken, ^{
                    keyWindow = [[app windows] firstObject];
                });

                UIView *keyView = [keyWindow hitTest:[rep location] withEvent:nil];

                UITouchPhase phase = UITouchPhaseEnded;
                if ([rep isTouchDown])
                    phase = UITouchPhaseBegan;
                else if ([rep isMove])
                    phase = UITouchPhaseMoved;
                else if ([rep isCancel])
                    phase = UITouchPhaseCancelled;
                else if ([rep isLift] || [rep isInRange] || [rep isInRangeLift])
                    phase = UITouchPhaseEnded;

                NSInteger pointerId = [[[[rep handInfo] paths] firstObject] pathIdentity];
                if (pointerId > 0)
                    [TSEventFetcher receiveAXEventID:MIN(MAX(pointerId, 1), 98) atGlobalCoordinate:[rep location] withTouchPhase:phase inWindow:keyWindow onView:keyView];
            });
        }
    }
}

int main(int argc, char *argv[])
{
    @autoreleasepool
    {
        if (argc <= 1) {
            return UIApplicationMain(argc, argv, @"MainApplication", @"MainApplicationDelegate");
        }

        if (strcmp(argv[1], "-hud") == 0)
        {
            pid_t pid = getpid();

            NSString *pidString = [NSString stringWithFormat:@"%d", pid];
            [pidString writeToFile:ROOT_PATH_NS(PID_PATH)
                        atomically:YES
                          encoding:NSUTF8StringEncoding
                             error:nil];

            [UIScreen initialize];
            CFRunLoopGetCurrent();

            GSInitialize();
            BKSDisplayServicesStart();
            UIApplicationInitialize();

            UIApplicationInstantiateSingleton(objc_getClass("HUDMainApplication"));
            static id<UIApplicationDelegate> appDelegate = [[objc_getClass("HUDMainApplicationDelegate") alloc] init];
            [UIApplication.sharedApplication setDelegate:appDelegate];
            [UIApplication.sharedApplication _accessibilityInit];

            [NSRunLoop currentRunLoop];
            BKSHIDEventRegisterEventCallback(_HUDEventCallback);

            if (@available(iOS 15.0, *)) {
                GSEventInitialize(0);
                GSEventPushRunLoopMode(kCFRunLoopDefaultMode);
            }

            [UIApplication.sharedApplication __completeAndRunAsPlugin];

            static int _springboardBootToken;
            notify_register_dispatch("SBSpringBoardDidLaunchNotification", &_springboardBootToken, dispatch_get_main_queue(), ^(int token) {
                notify_cancel(token);

                notify_post(NOTIFY_DESTROY_HUD);

                // Re-enable HUD after SpringBoard is launched.
                SetHUDEnabled(YES);

                // Exit the current instance of HUD.
                kill(pid, SIGKILL);
            });

            CFRunLoopRun();
            return EXIT_SUCCESS;
        }
    }
}
