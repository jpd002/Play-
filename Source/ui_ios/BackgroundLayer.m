//
//  BackgroundLayer.m
//  Play
//
//  Created by Lounge Katt on 8/17/15.
//  Copyright (c) 2015 Jean-Philip Desjardins. All rights reserved.
//

#import "BackgroundLayer.h"

@implementation BackgroundLayer

//Blue gradient background
+ (CAGradientLayer*) blueGradient {
	
	UIColor *colorOne = [UIColor colorWithRed:(21/255.0) green:(169/255.0) blue:(207/255.0) alpha:1.0];
	UIColor *colorTwo = [UIColor colorWithRed:(69/255.0)  green:(67/255.0)  blue:(142/255.0)  alpha:1.0];
	
	NSArray *colors = [NSArray arrayWithObjects:(id)colorOne.CGColor, colorTwo.CGColor, nil];
	NSNumber *stopOne = [NSNumber numberWithFloat:0.0];
	NSNumber *stopTwo = [NSNumber numberWithFloat:1.0];
	
	NSArray *locations = [NSArray arrayWithObjects:stopOne, stopTwo, nil];
	
	CAGradientLayer *headerLayer = [CAGradientLayer layer];
	headerLayer.colors = colors;
	headerLayer.locations = locations;
	
	return headerLayer;
}

@end
