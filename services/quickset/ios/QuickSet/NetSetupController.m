//
//  NetSetupController.m
//  DOEDemo
//
//  Created by Patrick Lazik on 5/5/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import "NetSetupController.h"
#import "AppDelegate.h"

@interface NetSetupController ()

@end

@implementation NetSetupController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [_WiredView setHidden:NO];
    [_WiredManualView setHidden:YES];
    [_WirelessView setHidden:YES];
    [_WirelessManualView setHidden:YES];
    arrStatus = [[NSArray alloc] initWithObjects:@"WEP", @"WPA", @"WPA2-Personal", @"WPA2-Enterprise", nil];
    
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
    //One column
    return 1;
}

-(NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
    //set number of rows
    return arrStatus.count;
}

-(NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
    //set item per row
    return [arrStatus objectAtIndex:row];
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

- (IBAction)WiredWirelessSwitch:(UISegmentedControl*)sender {
    switch(sender.selectedSegmentIndex){
        case 0:
            [_WirelessView setHidden:YES];
            [_WiredView setHidden:NO];
            break;
        case 1:
            [_WirelessView setHidden:NO];
            [_WiredView setHidden:YES];
        default:
            break;
    }
}
- (IBAction)WirelessDHCPSwitch:(UISegmentedControl*)sender {
    switch(sender.selectedSegmentIndex){
        case 0:
            [_WirelessManualView setHidden:YES];
            break;
        case 1:
            [_WirelessManualView setHidden:NO];
        default:
            break;
    }

}

- (IBAction)WiredDHCPSwitch:(UISegmentedControl*)sender {
    switch(sender.selectedSegmentIndex){
        case 0:
            [_WiredManualView setHidden:YES];
            break;
        case 1:
            [_WiredManualView setHidden:NO];
        default:
            break;
    }
}

- (IBAction)Done:(id)sender {
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [appDelegate sendMessage:@"Controller \"DemoController\" online!"];
    [appDelegate sendMessage:@"XMPP server online!"];
    [self performSegueWithIdentifier:@"NetSetupControllerDone" sender:self];
    
}
@end
