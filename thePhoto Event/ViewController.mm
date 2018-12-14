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
{
    CFReadStreamRef rStr;
    CFWriteStreamRef wStr;
}

enum State {
    Idle,
    Opening,
    Open,
    Authenticated
};

@property (weak, nonatomic) IBOutlet UIButton *connectButton;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *spinner;
@property (weak, nonatomic) IBOutlet UITextField *AddressBox;
@property (weak, nonatomic) IBOutlet UIButton *sendImageButton;

@property NSInputStream *readStream;
@property NSOutputStream *writeStream;
@property BlockList *blockList;
@property (strong) NSMutableData *unprocessedData;

@property State currentState;
@property Boolean writingBlock;

@property NSDate* lastPing;
@property NSTimer* pingTimer;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    _currentState = Idle;
    _writingBlock = NO;
    _unprocessedData = [NSMutableData data];
    _pingTimer = nil;

    _blockList = new BlockList();
    _blockList->setNewBlockAvailableCallback([=] {
        [self writeNextBlock];
    });
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
    
    
    CFStreamCreatePairWithSocketToHost(kCFAllocatorDefault, (__bridge CFStringRef) ipAddress, 26157, &rStr, &wStr);
    
    NSDictionary* sslSettings = [[NSDictionary alloc] initWithObjectsAndKeys:
                                 [NSNumber numberWithBool:NO], kCFStreamSSLValidatesCertificateChain,
                                 @"localhost", kCFStreamSSLPeerName,
                                 kCFStreamSocketSecurityLevelNegotiatedSSL, kCFStreamSSLLevel,
                                 nil ];
    
    CFReadStreamSetProperty(rStr, kCFStreamPropertySSLSettings, (CFTypeRef) sslSettings);
    CFWriteStreamSetProperty(wStr, kCFStreamPropertySSLSettings, (CFTypeRef) sslSettings);
    
    _readStream = (__bridge NSInputStream *)(rStr);
    _writeStream = (__bridge NSOutputStream *)(wStr);
    
    _readStream.delegate = self;
    _writeStream.delegate = self;
    [_readStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [_writeStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    
    [_readStream open];
    [_writeStream open];
    
    //Show box connecting to the server
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Connecting to the server", nil) message:nil preferredStyle:UIAlertControllerStyleAlert];
    
    UIActivityIndicatorView* activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    [activityIndicator startAnimating];
    [[alert view] addSubview:activityIndicator];
    [self presentViewController:alert animated:YES completion:nil];
    
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

- (void)stream:(NSStream *)str handleEvent:(NSStreamEvent)eventCode {
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
            } else {
                [self dismissViewControllerAnimated:YES completion:^() {
                    NSError* err = [str streamError];
                    
                    UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Stream Error occurred.", nil) message:[err localizedDescription] preferredStyle:UIAlertControllerStyleAlert];
                    
                    UIAlertAction* action = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
                        //Do nothing
                    }];
                    
                    [alert addAction:action];
                    [self presentViewController:alert animated:YES completion:nil];
                }];
                
            }
            break;
        case NSStreamEventOpenCompleted:
            if (_currentState == Opening) {
                _blockList->pushBlock(BlockObjC::getCommand(@"HELLO"));
                _currentState = Open;
            }
            break;
        case NSStreamEventEndEncountered:
            _currentState = Idle;
            break;
        case NSStreamEventHasSpaceAvailable:
            if (str == _writeStream) {
                _writingBlock = NO;
                [self writeNextBlock];
                
                //NSData *data = [[NSData alloc] initWithData:[@"HELLO\n" dataUsingEncoding:NSASCIIStringEncoding]];
                //[_writeStream write:(uint8_t*)[data bytes] maxLength:[data length]];
            }
            break;
        case NSStreamEventHasBytesAvailable:
            [self readInMessage];
            break;
    }
}

- (void) readInMessage {
    uint8_t buffer[2048];
    NSInteger len = [_readStream read:buffer maxLength:2048];
    if (len != 0) {
        [_unprocessedData appendBytes:(const void*)buffer length:len];
        
        NSString* data = [[NSString alloc] initWithData:_unprocessedData encoding:NSUTF8StringEncoding];
        NSLog(@"%@", data);
        
        NSArray<NSString*>* dataParts = [data componentsSeparatedByString:@"\n"];
        for (NSString* data : dataParts) {
            if (_currentState == Authenticated) {
                if ([data isEqualToString:@"PING"]) {
                    //Reply with a ping
                    _blockList->pushBlock(BlockObjC::getCommand(@"PING"));
                    _lastPing = [[NSDate alloc] init];
                    
                    if (_pingTimer == nil) {
                        //Set up a new ping timer
                        _pingTimer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(ping) userInfo:nil repeats:YES];
                    }
                }
            } else if (_currentState == Open) {
                //Ensure this is a handshake okay message
                if ([data isEqualToString:@"HANDSHAKE OK"]) {
                    _currentState = Authenticated;
                    [self dismissViewControllerAnimated:YES completion:nil];
                    
                    //Send identify command
                    _blockList->pushBlock(BlockObjC::getCommand([@"IDENTIFY " stringByAppendingString:[[UIDevice currentDevice] name]]));
                    
                    //Register for pings
                    _blockList->pushBlock(BlockObjC::getCommand(@"REGISTERPING"));
                } else {
                    //We've got an error :(
                    [_readStream close];
                    [_writeStream close];
                    
                    [self dismissViewControllerAnimated:YES completion:^() {
                        UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"That didn't work.", nil) message:NSLocalizedString(@"We can't seem to get to that computer. Check that you've got the code correct and that you're connected to the correct WiFi network.", nil) preferredStyle:UIAlertControllerStyleAlert];
                        
                        UIAlertAction* action = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
                            //Do nothing
                        }];
                        
                        [alert addAction:action];
                        [self presentViewController:alert animated:YES completion:nil];
                        
                        _currentState = Idle;
                    }];
                }
            }
        }
    }
}

- (void) writeNextBlock {
    if (_writingBlock == YES) return; //Don't do anything
    if (_blockList->blockCount() == 0) return;
    if ([_writeStream hasSpaceAvailable] == NO) return;
    
    //Send the next block
    _writingBlock = YES;
    
    Block* b = _blockList->takeBlock();
    NSData* data = b->d->getData();
    
    uint8_t buf[[data length]];
    memcpy(buf, [data bytes], [data length]);
    [_writeStream write:buf maxLength:[data length]];
    
    //delete b at some point
    delete b;
}

- (void) ping {
    if ([[[NSDate alloc] init] timeIntervalSinceDate:_lastPing] > 30.0) {
        //We're disconnected folks!
        [_pingTimer invalidate];
        _pingTimer = nil;
        
        UIAlertController* alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Disconnected from the server", nil) message:NSLocalizedString(@"You've been disconnected from the server.", nil) preferredStyle:UIAlertControllerStyleAlert];
        
        UIAlertAction* action = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
            //Do nothing
        }];
        
        [alert addAction:action];
        [self presentViewController:alert animated:YES completion:nil];
        
        _blockList->clearBlocks();
        _currentState = Idle;
    }
}

@end
