//
//  DeviceMeta.h
//  DOEDemo
//
//  Created by Patrick Lazik on 5/8/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface DeviceMeta : UIViewController{
    NSArray *arrStatus;
}
- (IBAction)Done:(id)sender;
@property (weak, nonatomic) IBOutlet UILabel *DeviceText;
@property (weak, nonatomic) IBOutlet UIImageView *DeviceImage;
@property (weak, nonatomic) IBOutlet UIPickerView *DeviceDirectory;
@property (weak, nonatomic) IBOutlet UIScrollView *ScrollView;

@end
