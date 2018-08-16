#!/usr/bin/perl

# Run with --help or --man for usage

use Term::ANSIColor;
use Term::ReadKey;
use Getopt::Long;
use Pod::Usage;
use POSIX qw(strftime);

my $arg_loop;
my $arg_exit_error;
my $arg_sleep=5;
my $help;
my $man;

GetOptions ('help|?' => \$help, man => \$man, 'loop' => \$arg_loop, 'loop-sleep=n' => \$arg_sleep,'exit-on-error' => \$arg_exit_error) or pod2usage(2);
pod2usage(1) if $help;
pod2usage(-exitval => 0, -verbose => 2) if $man;

my $osname = $^O;

while(1)
{
    if($osname eq 'MSWin32'){system("cls");}
    else{system("clear");}

    my @tests = ();
    my @files = glob "test/*.lake";
    for (0..$#files)
    {
        open my $file, '<', $files[$_];
        my $firstLine = <$file>;
        close $file;

        # Trim and check if first line indicates a wish for  test-inclusion
        $firstLine =~ s/^\s+|\s+$//g;
        if($firstLine eq "#AUTOTEST")
        {
            push @tests, $files[$_]
        }
    }

    my $cnt = scalar @tests;
    print("Running $cnt tests\n");

    my $failures = 0;
    my $successes = 0;

    for (0..$#tests)
    {
        my $fname = $tests[$_];
        my $runner = "out/lake -s $fname -r -t 2";

        use IPC::Open3;

        my($wtr, $rdr, $err);
        $pid = open3($wtr, $rdr, $err,$runner);

        waitpid( $pid, 0 );

        my $child_exit_status = $? >> 8;

        if ($? == 11)
        {
            print(colored("Segfault: $tests[$_]\n", 'red'));
        }
        elsif ($child_exit_status != 0)
        {
            print(colored("Failure: $tests[$_]\n", 'red'));
            $failures+=1;

            # On failures, print output indented
            my @lines = readline $rdr;
            splice @lines, 0, -6;

            print "    ...\n";
            print map {"    $_"} @lines;
            print("\n");

            exit 0 if ($arg_exit_error);

        }
        else
        {
            print colored( ("Success: $tests[$_]\n", 'green'));
            $successes++;
        }
    }

    print("$failures failed, $successes succeeded\n");

    # Exit if --loop is not set
    exit 0 if !$arg_loop;

    my $next_run = time() + $arg_sleep;
    print "Next run: ";
    print POSIX::strftime('%T', localtime($next_run)); #%Y-%m-%d %T
    print " (press Enter to run immediately)\n";

    # Otherwise, sleep a bit before continuing. Keypresses cancels the sleep.
    ReadKey $arg_sleep;

} # while

__END__
=head1 NAME

runtests - Run tests

=head1 SYNOPSIS

runtests.pl [options]

    Options:

        --loop              Keep running tests
        --loop-sleep M      Sleep for N seconds between
                            iterations (default 5)
        --exit-on-error     If a test fails, exit the
                            test runner
        --man               Show manual
        --help              This page

=head1 DESCRIPTION

    This programs collects all *.pa files in the "test" directory, relative
    to the current directory, whose first line is #AUTOTEST.

    For each matching file, run it using Planck interpreter. The interpreter
    returns non-zero exit codes on error and asserts, which is then reported
    as a test failure.

=cut
