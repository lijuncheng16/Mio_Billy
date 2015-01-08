//
//  GatewayBLEVC.m
//  DOEDemo
//
//  Created by Patrick Lazik on 5/6/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import "GatewayBLEVC.h"
#import "AppDelegate.h"

@interface GatewayBLEVC ()

@end

@implementation GatewayBLEVC

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
    _StatusLabel.text = @"Please select a gateway to register:";
    controllers = [[NSMutableArray alloc] initWithObjects:@"DemoGateway", nil];
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
    //AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    //[appDelegate sendMessage:@"Gateway \"DemoGateway\" requesting access"];
    [self performSegueWithIdentifier:@"GatewayQRSegue" sender:self];
}


/*
 #pragma mark - Navigation
 
 // In a storyboard-based application, you will often want to do a little preparation before navigation
 - (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
 {
 // Get the new view controller using [segue destinationViewController].
 // Pass the selected object to the new view controller.
 }
 */

@end
