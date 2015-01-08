//
//  DeviceGatewayScan.h
//  DOEDemo
//
//  Created by Patrick Lazik on 5/6/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface DeviceGatewayScan : UIViewController
{
    NSMutableArray *controllers;
    NSInteger rows;
}

@property (weak, nonatomic) IBOutlet UITableView *ControllerTable;
@property (weak, nonatomic) IBOutlet UILabel *StatusLabel;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *StatusWheel;

@end