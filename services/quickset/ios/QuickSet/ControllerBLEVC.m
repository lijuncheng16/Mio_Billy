//
//  ControllerBLEVC.m
//  DOEDemo
//
//  Created by Patrick Lazik on 5/4/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import "ControllerBLEVC.h"
#import "QRScanner.h"


#define BBB_HRM_DEVICE_INFO_SERVICE_UUID @"180A"
#define BBB_HRM_MANUFACTURER_NAME_CHARACTERISTIC_UUID @"2A29"

@interface ControllerBLEVC ()

@end

@implementation ControllerBLEVC 

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    rows = 0;
    [self.ControllerTable registerClass:[UITableViewCell class] forCellReuseIdentifier:@"Cell"];
    _StatusWheel.hidesWhenStopped = YES;
    [_StatusWheel startAnimating];
    
    // Scan for all available CoreBluetooth LE devices and connect
    NSArray *services = @[[CBUUID UUIDWithString:BBB_HRM_MANUFACTURER_NAME_CHARACTERISTIC_UUID], [CBUUID UUIDWithString:BBB_HRM_DEVICE_INFO_SERVICE_UUID]];
    CBCentralManager *centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
    [centralManager scanForPeripheralsWithServices:services options:nil];
    [NSTimer scheduledTimerWithTimeInterval:3 target:self selector:@selector( scanningComplete:)userInfo:nil repeats:NO];
}
                                                                                      
- (void)scanningComplete:(NSTimer*)timer
    {
        [_StatusWheel stopAnimating];
        _StatusLabel.textAlignment = NSTextAlignmentCenter;
        _StatusLabel.text = @"Please select a controller to register:";
        controllers = [[NSMutableArray alloc] initWithObjects:@"DemoController", nil];
        NSIndexPath *path1 = [NSIndexPath indexPathForRow:0 inSection:0];
        NSArray *indexArray = [NSArray arrayWithObjects:path1, nil];
        
        [_ControllerTable beginUpdates];
        [_ControllerTable insertRowsAtIndexPaths:indexArray withRowAnimation: UITableViewRowAnimationTop];
        [_ControllerTable endUpdates];
      
    }

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"Cell" forIndexPath:indexPath];
    
    // Configure the cell...
    NSString *controllerName = [controllers objectAtIndex:0];
    cell.textLabel.text = controllerName;
    
    return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return rows++;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [self performSegueWithIdentifier:@"ControllerQRSegue" sender:self];
}


// BT Related functions
#pragma mark - CBCentralManagerDelegate

// method called whenever you have successfully connected to the BLE peripheral
- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
}

// CBCentralManagerDelegate - This is called with the CBPeripheral class as its main input parameter. This contains most of the information there is to know about a BLE peripheral.
- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI
{
}

// method called whenever the device state changes.
- (void)centralManagerDidUpdateState:(CBCentralManager *)central
{
}

#pragma mark - CBPeripheralDelegate

// CBPeripheralDelegate - Invoked when you discover the peripheral's available services.
- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error
{
}

// Invoked when you discover the characteristics of a specified service.
- (void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error
{
}

// Invoked when you retrieve a specified characteristic's value, or when the peripheral device notifies your app that the characteristic's value has changed.
- (void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
}

@end
