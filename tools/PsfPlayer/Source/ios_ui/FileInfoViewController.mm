#import "FileInfoViewController.h"
#import "NSStringUtils.h"

enum
{
	SECTION_INFO,
	SECTION_RAWTAGS,
	SECTION_COUNT
};

enum
{
	INFO_SECTION_TITLE,
	INFO_SECTION_ARTIST,
	INFO_SECTION_GAME,
	INFO_SECTION_YEAR,
	INFO_SECTION_GENRE,
	INFO_SECTION_COMMENT,
	INFO_SECTION_COPYRIGHT,
	INFO_SECTION_PSFBY,
	INFO_SECTION_COUNT
};

@implementation FileInfoViewController

@synthesize delegate;

-(void)setTags: (const CPsfTags&)tags
{
	m_tags = tags;
	m_rawTags.clear();
	
	for(CPsfTags::ConstTagIterator tagIterator(m_tags.GetTagsBegin());
		tagIterator != m_tags.GetTagsEnd(); tagIterator++)
	{
		std::string rawTag;
		rawTag = tagIterator->first + ": " + tagIterator->second;
		m_rawTags.push_back(rawTag);
	}
	
	[m_tagsTable reloadData];
}

-(void)setTrackTitle: (NSString*)trackTitle
{
	m_trackTitleLabel.text = trackTitle;
}

-(void)setTrackTime: (NSString*)trackTime
{
	m_trackTimeLabel.text = trackTime;
}

-(void)setPlayButtonText: (NSString*)playButtonText
{
	[m_playButton setTitle: playButtonText forState: UIControlStateNormal];
}

-(IBAction)onPlayButtonPress: (id)sender
{
	if(delegate != nil)
	{
		[self.delegate onPlayButtonPress];
	}
}

-(IBAction)onPrevButtonPress: (id)sender
{
	if(delegate != nil)
	{
		[self.delegate onPrevButtonPress];
	}
}

-(IBAction)onNextButtonPress: (id)sender
{
	if(delegate != nil)
	{
		[self.delegate onNextButtonPress];
	}
}

-(NSInteger)numberOfSectionsInTableView: (UITableView*)tableView 
{
	return SECTION_COUNT;
}

-(NSInteger)tableView: (UITableView*)tableView numberOfRowsInSection: (NSInteger)section 
{
	NSInteger rowCount = 0;
	switch(section)
	{
		case SECTION_INFO:
			rowCount = INFO_SECTION_COUNT;
			break;
		case SECTION_RAWTAGS:
			rowCount = m_rawTags.size();
			break;
	}
	return rowCount;
}

-(NSString*)tableView: (UITableView*)tableView titleForHeaderInSection: (NSInteger)section 
{
	NSString *sectionHeader = nil;
	
	switch(section)
	{
		case SECTION_INFO:
			sectionHeader = @"Info";
			break;
		case SECTION_RAWTAGS:
			sectionHeader = @"Raw Tags";
			break;
	}
	
	return sectionHeader;
}

-(NSString*)infoDisplayTextForItem: (const char*)itemName withTag: (const char*)tagName
{
	NSString* itemValue = @"";
	if(m_tags.HasTag(tagName))
	{
		itemValue = stringWithWchar(m_tags.GetTagValue(tagName));
	}
	return [NSString stringWithFormat: @"%s: %@", itemName, itemValue];
}

-(UITableViewCell*)tableView: (UITableView*)tableView cellForRowAtIndexPath: (NSIndexPath*)indexPath 
{
	static NSString* CellIdentifier = @"Cell";
	
	UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier: CellIdentifier];
	if(cell == nil)
	{
		cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleDefault reuseIdentifier: CellIdentifier] autorelease];
	}
	
	cell.textLabel.adjustsFontSizeToFitWidth = YES;
	
	if(indexPath.section == SECTION_INFO)
	{
		switch(indexPath.row)
		{
		case INFO_SECTION_TITLE:
			cell.textLabel.text = [self infoDisplayTextForItem: "Title" withTag: "title"];
			break;
		case INFO_SECTION_ARTIST:
			cell.textLabel.text = [self infoDisplayTextForItem: "Artist" withTag: "artist"];
			break;
		case INFO_SECTION_GAME:
			cell.textLabel.text = [self infoDisplayTextForItem: "Game" withTag: "game"];
			break;
		case INFO_SECTION_YEAR:
			cell.textLabel.text = [self infoDisplayTextForItem: "Year" withTag: "year"];
			break;
		case INFO_SECTION_GENRE:
			cell.textLabel.text = [self infoDisplayTextForItem: "Genre" withTag: "genre"];
			break;
		case INFO_SECTION_COMMENT:
			cell.textLabel.text = [self infoDisplayTextForItem: "Comment" withTag: "comment"];
			break;
		case INFO_SECTION_COPYRIGHT:
			cell.textLabel.text = [self infoDisplayTextForItem: "Copyright" withTag: "copyright"];
			break;
		case INFO_SECTION_PSFBY:
			cell.textLabel.text = [self infoDisplayTextForItem: "Psf by" withTag: "psfby"];
			break;
		}
	}
	else if(indexPath.section == SECTION_RAWTAGS)
	{
		std::string rawTag = m_rawTags[indexPath.row];
		cell.textLabel.text = [NSString stringWithUTF8String: rawTag.c_str()];
	}
	
	return cell;
}

@end
