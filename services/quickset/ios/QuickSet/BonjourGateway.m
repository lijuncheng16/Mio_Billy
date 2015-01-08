//
//  BonjourGateway.m
//  DOEDemo
//
//  Created by Patrick Lazik on 5/6/14.
//  Copyright (c) 2014 CMU. All rights reserved.
//

#import "BonjourGateway.h"
#import "AppDelegate.h"
#import "BonjourBrowser.h"

@interface BonjourGateway ()

@end

@implementation BonjourGateway

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
    // Scan using Bonjour
    BonjourBrowser *bonjour;
    [bonjour loadView];
    _StatusWheel.hidesWhenStopped = YES;
    [_StatusWheel startAnimating];
    [NSTimer scheduledTimerWithTimeInterval:3 target:self selector:@selector( scanningComplete:)userInfo:nil repeats:NO];
}

- (void)scanningComplete:(NSTimer*)timer
{
    [_StatusWheel stopAnimating];
    _StatusLabel.textAlignment = NSTextAlignmentCenter;
    _StatusLabel.text = @"Please select a controller to register the gateway to:";
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
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [appDelegate sendMessage:@"Gateway \"DemoGateway\" requesting access"];

    [self performSegueWithIdentifier:@"GatewayAliasSegue" sender:self];
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
