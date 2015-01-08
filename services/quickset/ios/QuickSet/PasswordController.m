//
//  PasswordController.m
//  DOEDemo
//
//  Created by Patrick Lazik on 5/5/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import "PasswordController.h"
#import "AppDelegate.h"

@interface PasswordController ()

@end

@implementation PasswordController

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

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

- (IBAction)DoneButton:(id)sender {
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [appDelegate sendMessage:@"Admin password set successfully"];
    UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:@"Add controller" message:@"Registering controller..." delegate:self cancelButtonTitle:nil otherButtonTitles:nil];
    [alertView show];
    [self performSelector:@selector(dismissAlertViewInitial:) withObject:alertView afterDelay:2];
}

-(void)dismissAlertViewInitial:(UIAlertView *)alert{
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [appDelegate sendMessage:@"Web portal launched"];
    [alert dismissWithClickedButtonIndex:0 animated:YES];
    UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:@"Success"
                                                        message:@"Controller registered succesfully!"
                                                       delegate:self
                                              cancelButtonTitle:nil
                                              otherButtonTitles:nil];
    [alertView show];
    [self performSelector:@selector(dismissAlertView:) withObject:alertView afterDelay:2];
}

-(void)dismissAlertView:(UIAlertView *)alertView{
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [appDelegate sendMessage:@"Gateway PnP agent launched"];
    [alertView dismissWithClickedButtonIndex:0 animated:YES];
    [self performSegueWithIdentifier:@"ControllerDoneSegue" sender:self];
}
@end