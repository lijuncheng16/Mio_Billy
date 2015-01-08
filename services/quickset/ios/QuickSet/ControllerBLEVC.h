//
//  ControllerBLEVC.h
//  DOEDemo
//
//  Created by Patrick Lazik on 5/4/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import <UIKit/UIKit.h>
@import CoreBluetooth;

@interface ControllerBLEVC : UIViewController <UITableViewDataSource, UITableViewDelegate>
{
    NSMutableArray *controllers;
    NSInteger rows;
    
}

@property (nonatomic, strong) CBCentralManager *centralManager;
@property (nonatomic, strong) CBPeripheral     *BBBPeripheral;
@property (weak, nonatomic) IBOutlet UITableView *ControllerTable;
@property (weak, nonatomic) IBOutlet UILabel *StatusLabel;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *StatusWheel;

@end
