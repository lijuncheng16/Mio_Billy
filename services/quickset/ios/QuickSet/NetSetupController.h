//
//  NetSetupController.h
//  DOEDemo
//
//  Created by Patrick Lazik on 5/5/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface NetSetupController : UIViewController <UIPickerViewDataSource,UIPickerViewDelegate>{
    NSArray *arrStatus;
}
- (IBAction)WiredWirelessSwitch:(id)sender;
@property (weak, nonatomic) IBOutlet UIScrollView *WiredView;
@property (weak, nonatomic) IBOutlet UIScrollView *WirelessView;
@property (weak, nonatomic) IBOutlet UIView *WirelessManualView;
@property (weak, nonatomic) IBOutlet UIView *WiredManualView;
@property (weak, nonatomic) IBOutlet UIPickerView *SecurityPicker;
- (IBAction)WirelessDHCPSwitch:(id)sender;
- (IBAction)WiredDHCPSwitch:(id)sender;
- (IBAction)Done:(id)sender;

@end
