//
//  DeviceMeta.m
//  DOEDemo
//
//  Created by Patrick Lazik on 5/8/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import "DeviceMeta.h"
#import "AppDelegate.h"

@interface DeviceMeta ()

@end

@implementation DeviceMeta

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

-(void) hideKeyBoard:(id) sender
{
    //UITouch *touch = [[event allTouches] anyObject];
    //if ([touch view] != _DeviceText) {
    [self.view endEditing:YES];
    //}
    //[super touchesBegan:touches withEvent:event];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    //arrStatus = [[NSArray alloc] initWithObjects:@"Root", @"Floor 1", @"Floor 2", nil];
    switch([appDelegate getDevice]){
        case DEVICE_FFSENSOR:
            _DeviceText.text = @"FireFly Environmental Sensor";
            _DeviceImage.image = [UIImage imageNamed:@"firefly.png"];
            break;
        case DEVICE_FFPLUG:
            _DeviceText.text = @"FireFly Plug Meter";
            _DeviceImage.image = [UIImage imageNamed:@"plug.jpg"];
            break;
        case DEVICE_LUTRON:
            _DeviceText.text = @"Lutron Quantum";
            _DeviceImage.image = [UIImage imageNamed:@"lutron.jpg"];
            break;
            
        default:
             self.DeviceText.text = @"";
            break;
    }
    [_DeviceImage setNeedsDisplay];
   // UITapGestureRecognizer *gestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(hideKeyBoard:)];
   // [_TouchView addGestureRecognizer:gestureRecognizer];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


//
//-(NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
//{
//    //One column
//    return 1;
//}
//
//-(NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
//{
//    //set number of rows
//    return arrStatus.count;
//}
//
//-(NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
//{
//    //set item per row
//    return [arrStatus objectAtIndex:row];
//}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

- (IBAction)Done:(id)sender {
    [[NSOperationQueue mainQueue] addOperationWithBlock:^ {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Add device" message:@"Registering device..." delegate:self cancelButtonTitle:nil otherButtonTitles:nil];
        [alert show];
            [self performSelector:@selector(dismissAlertViewInitial:) withObject:alert afterDelay:2];
            }];

}

-(void)dismissAlertViewInitial:(UIAlertView *)alert{
    [alert dismissWithClickedButtonIndex:0 animated:YES];
    UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:@"Success"
                                                        message:@"Device registered succesfully!"
                                                       delegate:self
                                              cancelButtonTitle:nil
                                              otherButtonTitles:nil];
    [alertView show];
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    //[appDelegate sendMessage:@"Device \"Firefly Environmental Sensor\" added succesfully!"];
    
    [appDelegate sendMessage:[NSString stringWithFormat:@"%u", [appDelegate getDevice]]];

    [self performSelector:@selector(dismissAlertView:) withObject:alertView afterDelay:2];
}

-(void)dismissAlertView:(UIAlertView *)alertView{
    [alertView dismissWithClickedButtonIndex:0 animated:YES];
    [self performSegueWithIdentifier:@"DeviceDone" sender:self];

    
}
@end
