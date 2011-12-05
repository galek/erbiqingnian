# Simple script to merge unique lines of two text files.

sub Process
{
	my ($InputFile, $AdditionsFile, $OutputFile) = @_;

	open(FH, $InputFile) || die("unable to open input file: " . $InputFile);
	my @InputData = <FH>;
	close(FH);

	# The hash allows us to detect duplicate lines.
	my %UniqueLines;
	foreach my $line (@InputData)
	{
		$UniqueLines{$line} = 0;
	}

	open(FH, $AdditionsFile) || die("unable to open additions file: " . $AdditionsFile);
	my @AdditionsData = <FH>;
	close(FH);

	# Done this way to try to preserve order.
	foreach my $line (@AdditionsData)
	{
		if (!exists($UniqueLines{$line}))
		{
			push(@InputData, $line);
			$UniqueLines{$line} = 0;
		}
	}

	open(FH, ">$OutputFile") || die("unable to open output file: " . $OutputFile);
	print FH @InputData;
	close(FH);
}

Process($ARGV[0], $ARGV[1], $ARGV[2]);
