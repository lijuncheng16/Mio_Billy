//
//  GatewayAlias.m
//  DOEDemo
//
//  Created by Patrick Lazik on 5/6/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import "GatewayAlias.h"
#import "AppDelegate.h"

@interface GatewayAlias ()

@end

@implementation GatewayAlias

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
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)DoneButton:(id)sender {
    UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:@"Add gateway" message:@"Registering gateway..." delegate:self cancelButtonTitle:nil otherButtonTitles:nil];
    [alertView show];
    [self performSelector:@selector(dismissAlertViewInitial:) withObject:alertView afterDelay:2];
}

-(void)dismissAlertViewInitial:(UIAlertView *)alert{
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [appDelegate sendMessage:@"Sending validation to requesting user"];
    [alert dismissWithClickedButtonIndex:0 animated:YES];
    UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:@"Success"
                                                        message:@"Gateway registered succesfully!"
                                                       delegate:self
                                              cancelButtonTitle:nil
                                              otherButtonTitles:nil];
    [alertView show];
    [self performSelector:@selector(dismissAlertView:) withObject:alertView afterDelay:2];
}

-(void)dismissAlertView:(UIAlertView *)alertView{
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [appDelegate sendMessage:@"Gateway \"DemoGateway\" added succesfully!"];
    [alertView dismissWithClickedButtonIndex:0 animated:YES];
    [self performSegueWithIdentifier:@"GatewayDoneSegue" sender:self];
}
@end
