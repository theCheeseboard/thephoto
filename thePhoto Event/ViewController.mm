//
//  ViewController.m
//  thePhoto Event
//
//  Created by Victor Tran on 12/12/18.
//  Copyright © 2018 Victor Tran. All rights reserved.
//

#import "ViewController.h"
#include "BlockObjC.h"

@interface ViewController ()

enum State {
    Idle,
    Opening,
    Open
};

@property (weak, nonatomic) IBOutlet UIButton *connectButton;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *spinner;
@property (weak, nonatomic) IBOutlet UITextField *AddressBox;
@property (weak, nonatomic) IBOutlet UIButton *sendImageButton;

@property NSInputStream *readStream;
@property NSOutputStream *writeStream;
@property BlockList *blockList;

@property State currentState;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    _currentState = Idle;
    _blockList = new BlockList();
}

- (IBAction)connectButtonClicked:(UIButton *)sender {
    unsigned int ipCode = [[[_AddressBox text] stringByReplacingOccurrencesOfString:@" " withString:@""] longLongValue];
    if (ipCode == 0) {
        //Error
        UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"That didn't work.", nil) message:NSLocalizedString(@"We can't seem to get to that computer. Check that you've got the code correct and that you're connected to the correct WiFi network.", nil) preferredStyle:UIAlertControllerStyleAlert];
        
        UIAlertAction* action = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
            //Do nothing
        }];
        
        [alert addAction:action];
        [self presentViewController:alert animated:YES completion:nil];
        return;
    }
    
    unsigned int ipParts[4] = {
        (ipCode & 0xFF000000) >> 24,
        (ipCode & 0x00FF0000) >> 16,
        (ipCode & 0x0000FF00) >> 8,
        (ipCode & 0x000000FF)
    };
    
    
    NSString* ipAddress = [[NSArray arrayWithObjects:
                            [NSString stringWithFormat:@"%d", ipParts[0]],
                            [NSString stringWithFormat:@"%d", ipParts[1]],
                            [NSString stringWithFormat:@"%d", ipParts[2]],
                            [NSString stringWithFormat:@"%d", ipParts[3]], nil
                          ] componentsJoinedByString:@"."];
    
    
    CFReadStreamRef rStr;
    CFWriteStreamRef wStr;
    CFStreamCreatePairWithSocketToHost(kCFAllocatorDefault, (__bridge CFStringRef) ipAddress, 26157, &rStr, &wStr);
    
    NSDictionary* sslSettings = [[NSDictionary alloc] initWithObjectsAndKeys:
                                 [NSNumber numberWithBool:YES], @"kCFStreamSSLAllowsExpiredCertificates",
                                 [NSNumber numberWithBool:YES], @"kCFStreamSSLAllowsExpiredRoots",
                                 [NSNumber numberWithBool:YES], @"kCFStreamSSLAllowsAnyRoot",
                                 [NSNumber numberWithBool:NO], @"kCFStreamSSLValidatesCertificateChain",
                                 [NSNull null], @"kCFStreamSSLPeerName",
                                 @"kCFStreamSocketSecurityLevelNegotiatedSSL",
                                 @"kCFStreamSSLLevel",
                                 nil ];
    
    CFReadStreamSetProperty(rStr, (CFStringRef)@"kCFStreamPropertySSLSettings", (CFTypeRef) sslSettings);
    CFWriteStreamSetProperty(wStr, (CFStringRef)@"kCFStreamPropertySSLSettings", (CFTypeRef) sslSettings);
    
    _readStream = (__bridge NSInputStream *)(rStr);
    _writeStream = (__bridge NSOutputStream *)(wStr);
    
    _readStream.delegate = self;
    _writeStream.delegate = self;
    [_readStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [_writeStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    
    [_readStream open];
    [_writeStream open];
    
    
    //Error
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Connecting to the server", nil) message:nil preferredStyle:UIAlertControllerStyleAlert];
    
    UIActivityIndicatorView* activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    [activityIndicator startAnimating];
    [[alert view] addSubview:activityIndicator];
    [self presentViewController:alert animated:YES completion:nil];
    
    Block b = BlockObjC::getCommand(@"HELLO");
    _currentState = Opening;
}

- (IBAction)sendImageButtonClicked:(UIButton *)sender {
    if (_currentState != Open) {
        UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Connect to a server first", nil) message:NSLocalizedString(@"Before you can send images, you'll need to connect to thePhoto.", nil) preferredStyle:UIAlertControllerStyleAlert];
        
        UIAlertAction* action = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
            //Do nothing
        }];
        
        [alert addAction:action];
        [self presentViewController:alert animated:YES completion:nil];
        return;
    }
    
    UIImagePickerController* imagePicker = [[UIImagePickerController alloc] init];
    imagePicker.delegate = self;
    imagePicker.allowsEditing = NO;
    imagePicker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
    [self presentViewController:imagePicker animated:YES completion:nil];
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info {
    
    [picker dismissViewControllerAnimated:YES completion:nil];
    [_spinner startAnimating];
    
    UIImage* chosenImage = info[UIImagePickerControllerEditedImage];
    NSData* jpegData = UIImageJPEGRepresentation(chosenImage, 1.0);
}

- (void)stream:(NSStream *)aStream handleEvent:(NSStreamEvent)eventCode {
    switch (eventCode) {
        case NSStreamEventErrorOccurred:
            if (_currentState == Opening) {
                [self dismissViewControllerAnimated:YES completion:^() {
                    UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"That didn't work.", nil) message:NSLocalizedString(@"We can't seem to get to that computer. Check that you've got the code correct and that you're connected to the correct WiFi network.", nil) preferredStyle:UIAlertControllerStyleAlert];
                    
                    UIAlertAction* action = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
                        //Do nothing
                    }];
                    
                    [alert addAction:action];
                    [self presentViewController:alert animated:YES completion:nil];
                }];
                
                _currentState = Idle;
            }
            break;
        case NSStreamEventOpenCompleted:
            if (_currentState == Opening) {
                [self dismissViewControllerAnimated:YES completion:nil];
                _currentState = Open;
            }
            break;
        case NSStreamEventEndEncountered:
            _currentState = Idle;
            break;
    }
}

@end
