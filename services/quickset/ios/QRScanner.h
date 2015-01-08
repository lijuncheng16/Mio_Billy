//
//  QRScanner.h
//  DOEDemo
//
//  Created by Patrick Lazik on 5/5/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@interface QRScanner : UIViewController <AVCaptureMetadataOutputObjectsDelegate>
@property (nonatomic) BOOL isReading;
@property (nonatomic, strong) AVCaptureSession *captureSession;
@property (nonatomic, strong) AVCaptureVideoPreviewLayer *videoPreviewLayer;
@property (weak, nonatomic) IBOutlet UIView *viewPreview;
@property (nonatomic, strong) AVAudioPlayer *audioPlayer;

-(void)loadBeepSound;
-(BOOL)startReading;
@end
