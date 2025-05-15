#import <Foundation/Foundation.h>

@interface SimpleClass : NSObject
-(void)sampleMethod;
@end

@implementation SimpleClass
-(void)sampleMethod{
    NSLog(@"Hello, World!\n");
}
@end

int main(){
    SimpleClass *sc=[[SimpleClass alloc]init];
    [sc sampleMethod];
    return EXIT_SUCCESS;
}
