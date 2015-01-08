//
//  AppDelegate.h
//  DOEDemo
//
//  Created by Patrick Lazik on 5/4/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "XMPPFramework.h"
#import "XMPPPubSub.h"


enum device{DEVICE_FFSENSOR, DEVICE_FFPLUG, DEVICE_LUTRON};

@interface AppDelegate : UIResponder <UIApplicationDelegate>
{
	XMPPStream *xmppStream;
	XMPPReconnect *xmppReconnect;
    XMPPPubSub *xmppPubSub;
	
	NSString *password;
	
	BOOL customCertEvaluation;
	
	BOOL isXmppConnected;
	
	UIWindow *window;
    
    int currDevice;
}

@property (nonatomic, strong, readonly) XMPPStream *xmppStream;
@property (nonatomic, strong, readonly) XMPPReconnect *xmppReconnect;
@property (nonatomic, strong) IBOutlet UIWindow *window;

- (BOOL)connect;
- (void)disconnect;
- (void)sendMessage:(NSString*)message;
- (void)subscribe:(NSString*)node;
- (int)getDevice;
- (void)setDevice:(int)device;
@end

