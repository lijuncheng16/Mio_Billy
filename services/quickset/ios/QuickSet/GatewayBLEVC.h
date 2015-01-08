//
//  GatewayBLEVC.h
//  DOEDemo
//
//  Created by Patrick Lazik on 5/6/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import <UIKit/UIKit.h>
@import CoreBluetooth;

#define BBB_HRM_DEVICE_INFO_SERVICE_UUID @"180A"
#define BBB_HRM_MANUFACTURER_NAME_CHARACTERISTIC_UUID @"2A29"

@interface GatewayBLEVC : UIViewController
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
