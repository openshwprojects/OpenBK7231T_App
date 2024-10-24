/*
 * Copyright (c) 2002-2003 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 *
 * This code is completely 100% portable C. It does not depend on any external header files
 * from outside the mDNS project -- all the types it expects to find are defined right here.
 * 
 * The previous point is very important: This file does not depend on any external
 * header files. It should complile on *any* platform that has a C compiler, without
 * making *any* assumptions about availability of so-called "standard" C functions,
 * routines, or types (which may or may not be present on any given platform).

 * Formatting notes:
 * This code follows the "Whitesmiths style" C indentation rules. Plenty of discussion
 * on C indentation can be found on the web, such as <http://www.kafejo.com/komp/1tbs.htm>,
 * but for the sake of brevity here I will say just this: Curly braces are not syntactially
 * part of an "if" statement; they are the beginning and ending markers of a compound statement;
 * therefore common sense dictates that if they are part of a compound statement then they
 * should be indented to the same level as everything else in that compound statement.
 * Indenting curly braces at the same level as the "if" implies that curly braces are
 * part of the "if", which is false. (This is as misleading as people who write "char* x,y;"
 * thinking that variables x and y are both of type "char*" -- and anyone who doesn't
 * understand why variable y is not of type "char*" just proves the point that poor code
 * layout leads people to unfortunate misunderstandings about how the C language really works.)

 Change History (most recent first):

 $Log: mDNS.c,v $
 Revision 1.1.1.1  2005/07/23 13:57:05  shiro
 raop_play project

 Revision 1.1.2.2  2004/09/25 04:54:13  shiro
 *** empty log message ***

 Revision 1.1.2.1  2004/09/18 03:29:20  shiro
 *** empty log message ***

 Revision 1.307.2.8  2004/04/03 05:18:19  bradley
 Added cast to fix signed/unsigned warning due to int promotion.

 Revision 1.307.2.7  2004/03/30 06:46:24  cheshire
 Compiler warning fixes from Don Woodward at Roku Labs

 Revision 1.307.2.6  2004/03/09 03:03:38  cheshire
 <rdar://problem/3581961> Don't take lock until after mDNS_Update() has validated that the data is good

 Revision 1.307.2.5  2004/03/02 02:55:24  cheshire
 <rdar://problem/3549576> Properly support "_services._dns-sd._udp" meta-queries

 Revision 1.307.2.4  2004/02/18 01:55:08  cheshire
 <rdar://problem/3553472>: Increase delay to 400ms when answering queries with multi-packet KA lists

 Revision 1.307.2.3  2004/01/28 23:08:45  cheshire
 <rdar://problem/3488559>: Hard code domain enumeration functions to return ".local" only

 Revision 1.307.2.2  2003/12/20 01:51:40  cheshire
 <rdar://problem/3515876>: Error putting additional records into packets
 Another fix from Rampi: responseptr needs to be updated inside the "for" loop,
 after every record, not once at the end.

 Revision 1.307.2.1  2003/12/03 11:20:27  cheshire
 <rdar://problem/3457718>: Stop and start of a service uses old ip address (with old port number)

 Revision 1.307  2003/09/09 20:13:30  cheshire
 <rdar://problem/3411105> Don't send a Goodbye record if we never announced it
 Ammend checkin 1.304: Off-by-one error: By this place in the function we've already decremented
 rr->AnnounceCount, so the check needs to be for InitialAnnounceCount-1, not InitialAnnounceCount

 Revision 1.306  2003/09/09 03:00:03  cheshire
 <rdar://problem/3413099> Services take a long time to disappear when switching networks.
 Added two constants: kDefaultReconfirmTimeForNoAnswer and kDefaultReconfirmTimeForCableDisconnect

 Revision 1.305  2003/09/09 02:49:31  cheshire
 <rdar://problem/3413975> Initial probes and queries not grouped on wake-from-sleep

 Revision 1.304  2003/09/09 02:41:19  cheshire
 <rdar://problem/3411105> Don't send a Goodbye record if we never announced it

 Revision 1.303  2003/09/05 19:55:02  cheshire
 <rdar://problem/3409533> Include address records when announcing SRV records

 Revision 1.302  2003/09/05 00:01:36  cheshire
 <rdar://problem/3407549> Don't accelerate queries that have large KA lists

 Revision 1.301  2003/09/04 22:51:13  cheshire
 <rdar://problem/3398213> Group probes and goodbyes better

 Revision 1.300  2003/09/03 02:40:37  cheshire
 <rdar://problem/3404842> mDNSResponder complains about '_'s
 Underscores are not supposed to be legal in standard DNS names, but IANA appears
 to have allowed them in previous service name registrations, so we should too.

 Revision 1.299  2003/09/03 02:33:09  cheshire
 <rdar://problem/3404795> CacheRecordRmv ERROR
 Don't update m->NewQuestions until *after* CheckCacheExpiration();

 Revision 1.298  2003/09/03 01:47:01  cheshire
 <rdar://problem/3319418> Rendezvous services always in a state of flux
 Change mDNS_Reconfirm_internal() minimum timeout from 5 seconds to 45-60 seconds

 Revision 1.297  2003/08/29 19:44:15  cheshire
 <rdar://problem/3400967> Traffic reduction: Eliminate synchronized QUs when a new service appears
 1. Use m->RandomQueryDelay to impose a random delay in the range 0-500ms on queries
 that already have at least one unique answer in the cache
 2. For these queries, go straight to QM, skipping QU

 Revision 1.296  2003/08/29 19:08:21  cheshire
 <rdar://problem/3400986> Traffic reduction: Eliminate huge KA lists after wake from sleep
 Known answers are no longer eligible to go in the KA list if they are more than half-way to their expiry time.

 Revision 1.295  2003/08/28 01:10:59  cheshire
 <rdar://problem/3396034> Add syslog message to report when query is reset because of immediate answer burst

 Revision 1.294  2003/08/27 02:30:22  cheshire
 <rdar://problem/3395909> Traffic Reduction: Inefficiencies in DNSServiceResolverResolve()
 One more change: "query->GotTXT" is now a straightforward bi-state boolean again

 Revision 1.293  2003/08/27 02:25:31  cheshire
 <rdar://problem/3395909> Traffic Reduction: Inefficiencies in DNSServiceResolverResolve()

 Revision 1.292  2003/08/21 19:27:36  cheshire
 <rdar://problem/3387878> Traffic reduction: No need to announce record for longer than TTL

 Revision 1.291  2003/08/21 18:57:44  cheshire
 <rdar://problem/3387140> Synchronized queries on the network

 Revision 1.290  2003/08/21 02:25:23  cheshire
 Minor changes to comments and debugf() messages

 Revision 1.289  2003/08/21 02:21:50  cheshire
 <rdar://problem/3386473> Efficiency: Reduce repeated queries

 Revision 1.288  2003/08/20 23:39:30  cheshire
 <rdar://problem/3344098> Review syslog messages, and remove as appropriate

 Revision 1.287  2003/08/20 20:47:18  cheshire
 Fix compiler warning

 Revision 1.286  2003/08/20 02:18:51  cheshire
 <rdar://problem/3344098> Cleanup: Review syslog messages

 Revision 1.285  2003/08/20 01:59:06  cheshire
 <rdar://problem/3384478> rdatahash and rdnamehash not updated after changing rdata
 Made new routine SetNewRData() to update rdlength, rdestimate, rdatahash and rdnamehash in one place

 Revision 1.284  2003/08/19 22:20:00  cheshire
 <rdar://problem/3376721> Don't use IPv6 on interfaces that have a routable IPv4 address configured
 More minor refinements

 Revision 1.283  2003/08/19 22:16:27  cheshire
 Minor fix: Add missing "mDNS_Unlock(m);" in mDNS_DeregisterInterface() error case.

 Revision 1.282  2003/08/19 06:48:25  cheshire
 <rdar://problem/3376552> Guard against excessive record updates
 Each record starts with 10 UpdateCredits.
 Every update consumes one UpdateCredit.
 UpdateCredits are replenished at a rate of one one per minute, up to a maximum of 10.
 As the number of UpdateCredits declines, the number of announcements is similarly scaled back.
 When fewer than 5 UpdateCredits remain, the first announcement is also delayed by an increasing amount.

 Revision 1.281  2003/08/19 04:49:28  cheshire
 <rdar://problem/3368159> Interaction between v4, v6 and dual-stack hosts not working quite right
 1. A dual-stack host should only suppress its own query if it sees the same query from other hosts on BOTH IPv4 and IPv6.
 2. When we see the first v4 (or first v6) member of a group, we re-trigger questions and probes on that interface.
 3. When we see the last v4 (or v6) member of a group go away, we revalidate all the records received on that interface.

 Revision 1.280  2003/08/19 02:33:36  cheshire
 Update comments

 Revision 1.279  2003/08/19 02:31:11  cheshire
 <rdar://problem/3378386> mDNSResponder overenthusiastic with final expiration queries
 Final expiration queries now only mark the question for sending on the particular interface
 pertaining to the record that's expiring.

 Revision 1.278  2003/08/18 22:53:37  cheshire
 <rdar://problem/3382647> mDNSResponder divide by zero in mDNSPlatformTimeNow()

 Revision 1.277  2003/08/18 19:05:44  cheshire
 <rdar://problem/3382423> UpdateRecord not working right
 Added "newrdlength" field to hold new length of updated rdata

 Revision 1.276  2003/08/16 03:39:00  cheshire
 <rdar://problem/3338440> InterfaceID -1 indicates "local only"

 Revision 1.275  2003/08/16 02:51:27  cheshire
 <rdar://problem/3366590> mDNSResponder takes too much RPRVT
 Don't try to compute namehash etc, until *after* validating the name

 Revision 1.274  2003/08/16 01:12:40  cheshire
 <rdar://problem/3366590> mDNSResponder takes too much RPRVT
 Now that the minimum rdata object size has been reduced to 64 bytes, it is no longer safe to do a
 simple C structure assignment of a domainname, because that object is defined to be 256 bytes long,
 and in the process of copying it, the C compiler may run off the end of the rdata object into
 unmapped memory. All assignments of domainname objects of uncertain size are now replaced with a
 call to the macro AssignDomainName(), which is careful to copy only as many bytes as are valid.

 Revision 1.273  2003/08/15 20:16:02  cheshire
 <rdar://problem/3366590> mDNSResponder takes too much RPRVT
 We want to avoid touching the rdata pages, so we don't page them in.
 1. RDLength was stored with the rdata, which meant touching the page just to find the length.
 Moved this from the RData to the ResourceRecord object.
 2. To avoid unnecessarily touching the rdata just to compare it,
 compute a hash of the rdata and store the hash in the ResourceRecord object.

 Revision 1.272  2003/08/14 19:29:04  cheshire
 <rdar://problem/3378473> Include cache records in SIGINFO output
 Moved declarations of DNSTypeName() and GetRRDisplayString to mDNSClientAPI.h so daemon.c can use them

 Revision 1.271  2003/08/14 02:17:05  cheshire
 <rdar://problem/3375491> Split generic ResourceRecord type into two separate types: AuthRecord and CacheRecord

 Revision 1.270  2003/08/13 17:07:28  ksekar
 Bug #: <rdar://problem/3376458>: Extra RR linked to list even if registration fails - causes crash
 Added check to result of mDNS_Register() before linking extra record into list.

 Revision 1.269  2003/08/12 19:56:23  cheshire
 Update to APSL 2.0

 Revision 1.268  2003/08/12 15:01:10  cheshire
 Add comments

 Revision 1.267  2003/08/12 14:59:27  cheshire
 <rdar://problem/3374490> Rate-limiting blocks some legitimate responses
 When setting LastMCTime also record LastMCInterface. When checking LastMCTime to determine
 whether to suppress the response, also check LastMCInterface to see if it matches.

 Revision 1.266  2003/08/12 12:47:16  cheshire
 In mDNSCoreMachineSleep debugf message, display value of m->timenow

 Revision 1.265  2003/08/11 20:04:28  cheshire
 <rdar://problem/3366553> Improve efficiency by restricting cases where we have to walk the entire cache

 Revision 1.264  2003/08/09 00:55:02  cheshire
 <rdar://problem/3366553> mDNSResponder is taking 20-30% of the CPU
 Don't scan the whole cache after every packet.

 Revision 1.263  2003/08/09 00:35:29  cheshire
 Moved AnswerNewQuestion() later in the file, in preparation for next checkin

 Revision 1.262  2003/08/08 19:50:33  cheshire
 <rdar://problem/3370332> Remove "Cache size now xxx" messages

 Revision 1.261  2003/08/08 19:18:45  cheshire
 <rdar://problem/3271219> Only retrigger questions on platforms with the "PhantomInterfaces" bug

 Revision 1.260  2003/08/08 18:55:48  cheshire
 <rdar://problem/3370365> Guard against time going backwards

 Revision 1.259  2003/08/08 18:36:04  cheshire
 <rdar://problem/3344154> Only need to revalidate on interface removal on platforms that have the PhantomInterfaces bug

 Revision 1.258  2003/08/08 16:22:05  cheshire
 <rdar://problem/3335473> Need to check validity of TXT (and other) records
 Remove unneeded LogMsg

 Revision 1.257  2003/08/07 01:41:08  cheshire
 <rdar://problem/3367346> Ignore packets with invalid source address (all zeroes or all ones)

 Revision 1.256  2003/08/06 23:25:51  cheshire
 <rdar://problem/3290674> Increase TTL for A/AAAA/SRV from one minute to four

 Revision 1.255  2003/08/06 23:22:50  cheshire
 Add symbolic constants: kDefaultTTLforUnique (one minute) and kDefaultTTLforShared (two hours)

 Revision 1.254  2003/08/06 21:33:39  cheshire
 Fix compiler warnings on PocketPC 2003 (Windows CE)

 Revision 1.253  2003/08/06 20:43:57  cheshire
 <rdar://problem/3335473> Need to check validity of TXT (and other) records
 Created ValidateDomainName() and ValidateRData(), used by mDNS_Register_internal() and mDNS_Update()

 Revision 1.252  2003/08/06 20:35:47  cheshire
 Enhance debugging routine GetRRDisplayString() so it can also be used to display
 other RDataBody objects, not just the one currently attached the given ResourceRecord

 Revision 1.251  2003/08/06 19:07:34  cheshire
 <rdar://problem/3366251> mDNSResponder not inhibiting multicast responses as much as it should
 Was checking LastAPTime instead of LastMCTime

 Revision 1.250  2003/08/06 19:01:55  cheshire
 Update comments

 Revision 1.249  2003/08/06 00:13:28  cheshire
 Tidy up debugf messages

 Revision 1.248  2003/08/05 22:20:15  cheshire
 <rdar://problem/3330324> Need to check IP TTL on responses

 Revision 1.247  2003/08/05 00:56:39  cheshire
 <rdar://problem/3357075> mDNSResponder sending additional records, even after precursor record suppressed

 Revision 1.246  2003/08/04 19:20:49  cheshire
 Add kDNSQType_ANY to list in DNSTypeName() so it can be displayed in debugging messages

 Revision 1.245  2003/08/02 01:56:29  cheshire
 For debugging: log message if we ever get more than one question in a truncated packet

 Revision 1.244  2003/08/01 23:55:32  cheshire
 Fix for compiler warnings on Windows, submitted by Bob Bradley

 Revision 1.243  2003/07/25 02:26:09  cheshire
 Typo: FIxed missing semicolon

 Revision 1.242  2003/07/25 01:18:41  cheshire
 Fix memory leak on shutdown in mDNS_Close() (detected in Windows version)

 Revision 1.241  2003/07/23 21:03:42  cheshire
 Only show "Found record..." debugf message in verbose mode

 Revision 1.240  2003/07/23 21:01:11  cheshire
 <rdar://problem/3340584> Need Nagle-style algorithm to coalesce multiple packets into one
 After sending a packet, suppress further sending for the next 100ms.

 Revision 1.239  2003/07/22 01:30:05  cheshire
 <rdar://problem/3329099> Don't try to add the same question to the duplicate-questions list more than once

 Revision 1.238  2003/07/22 00:10:20  cheshire
 <rdar://problem/3337355> ConvertDomainLabelToCString() needs to escape escape characters

 Revision 1.237  2003/07/19 03:23:13  cheshire
 <rdar://problem/2986147> mDNSResponder needs to receive and cache larger records

 Revision 1.236  2003/07/19 03:04:55  cheshire
 Fix warnings; some debugf message improvements

 Revision 1.235  2003/07/19 00:03:32  cheshire
 <rdar://problem/3160248> ScheduleNextTask needs to be smarter after a no-op packet is received
 ScheduleNextTask is quite an expensive operation.
 We don't need to do all that work after receiving a no-op packet that didn't change our state.

 Revision 1.234  2003/07/18 23:52:11  cheshire
 To improve consistency of field naming, global search-and-replace:
 NextProbeTime    -> NextScheduledProbe
 NextResponseTime -> NextScheduledResponse

 Revision 1.233  2003/07/18 00:29:59  cheshire
 <rdar://problem/3268878> Remove mDNSResponder version from packet header and use HINFO record instead

 Revision 1.232  2003/07/18 00:11:38  cheshire
 Add extra case to switch statements to handle HINFO data for Get, Put and Display
 (In all but GetRDLength(), this is is just a fall-through to kDNSType_TXT)

 Revision 1.231  2003/07/18 00:06:37  cheshire
 To make code a little easier to read in GetRDLength(), search-and-replace "rr->rdata->u." with "rd->"

 Revision 1.230  2003/07/17 18:16:54  cheshire
 <rdar://problem/3319418> Rendezvous services always in a state of flux
 In preparation for working on this, made some debugf messages a little more selective

 Revision 1.229  2003/07/17 17:35:04  cheshire
 <rdar://problem/3325583> Rate-limit responses, to guard against packet flooding

 Revision 1.228  2003/07/16 20:50:27  cheshire
 <rdar://problem/3315761> Need to implement "unicast response" request, using top bit of qclass

 Revision 1.227  2003/07/16 05:01:36  cheshire
 Add fields 'LargeAnswers' and 'ExpectUnicastResponse' in preparation for
 <rdar://problem/3315761> Need to implement "unicast response" request, using top bit of qclass

 Revision 1.226  2003/07/16 04:51:44  cheshire
 Fix use of constant 'mDNSPlatformOneSecond' where it should have said 'InitialQuestionInterval'

 Revision 1.225  2003/07/16 04:46:41  cheshire
 Minor wording cleanup: The correct DNS term is "response", not "reply"

 Revision 1.224  2003/07/16 04:39:02  cheshire
 Textual cleanup (no change to functionality):
 Construct "c >= 'A' && c <= 'Z'" appears in too many places; replaced with macro "mDNSIsUpperCase(c)"

 Revision 1.223  2003/07/16 00:09:22  cheshire
 Textual cleanup (no change to functionality):
 Construct "((mDNSs32)rr->rroriginalttl * mDNSPlatformOneSecond)" appears in too many places;
 replace with macro "TicksTTL(rr)"
 Construct "rr->TimeRcvd + ((mDNSs32)rr->rroriginalttl * mDNSPlatformOneSecond)"
 replaced with macro "RRExpireTime(rr)"

 Revision 1.222  2003/07/15 23:40:46  cheshire
 Function rename: UpdateDupSuppressInfo() is more accurately called ExpireDupSuppressInfo()

 Revision 1.221  2003/07/15 22:17:56  cheshire
 <rdar://problem/3328394> mDNSResponder is not being efficient when doing certain queries

 Revision 1.220  2003/07/15 02:12:51  cheshire
 Slight tidy-up of debugf messages and comments

 Revision 1.219  2003/07/15 01:55:12  cheshire
 <rdar://problem/3315777> Need to implement service registration with subtypes

 Revision 1.218  2003/07/14 16:26:06  cheshire
 <rdar://problem/3324795> Duplicate query suppression not working right
 Refinement: Don't record DS information for a question in the first quarter second
 right after we send it -- in the case where a question happens to be accelerated by
 the maximum allowed amount, we don't want it to then be suppressed because the previous
 time *we* sent that question falls (just) within the valid duplicate suppression window.

 Revision 1.217  2003/07/13 04:43:53  cheshire
 <rdar://problem/3325169> Services on multiple interfaces not always resolving
 Minor refinement: No need to make address query broader than the original SRV query that provoked it

 Revision 1.216  2003/07/13 03:13:17  cheshire
 <rdar://problem/3325169> Services on multiple interfaces not always resolving
 If we get an identical SRV on a second interface, convert address queries to non-specific

 Revision 1.215  2003/07/13 02:28:00  cheshire
 <rdar://problem/3325166> SendResponses didn't all its responses
 Delete all references to RRInterfaceActive -- it's now superfluous

 Revision 1.214  2003/07/13 01:47:53  cheshire
 Fix one error and one warning in the Windows build

 Revision 1.213  2003/07/12 04:25:48  cheshire
 Fix minor signed/unsigned warnings

 Revision 1.212  2003/07/12 01:59:11  cheshire
 Minor changes to debugf messages

 Revision 1.211  2003/07/12 01:47:01  cheshire
 <rdar://problem/3324495> After name conflict, appended number should be higher than previous number

 Revision 1.210  2003/07/12 01:43:28  cheshire
 <rdar://problem/3324795> Duplicate query suppression not working right
 The correct cutoff time for duplicate query suppression is timenow less one-half the query interval.
 The code was incorrectly using the last query time plus one-half the query interval.
 This was only correct in the case where query acceleration was not in effect.

 Revision 1.209  2003/07/12 01:27:50  cheshire
 <rdar://problem/3320079> Hostname conflict naming should not use two hyphens
 Fix missing "-1" in RemoveLabelSuffix()

 Revision 1.208  2003/07/11 01:32:38  cheshire
 Syntactic cleanup (no change to funcationality): Now that we only have one host name,
 rename field "hostname1" to "hostname", and field "RR_A1" to "RR_A".

 Revision 1.207  2003/07/11 01:28:00  cheshire
 <rdar://problem/3161289> No more local.arpa

 Revision 1.206  2003/07/11 00:45:02  cheshire
 <rdar://problem/3321909> Client should get callback confirming successful host name registration

 Revision 1.205  2003/07/11 00:40:18  cheshire
 Tidy up debug message in HostNameCallback()

 Revision 1.204  2003/07/11 00:20:32  cheshire
 <rdar://problem/3320087> mDNSResponder should log a message after 16 unsuccessful probes

 Revision 1.203  2003/07/10 23:53:41  cheshire
 <rdar://problem/3320079> Hostname conflict naming should not use two hyphens

 Revision 1.202  2003/07/04 02:23:20  cheshire
 <rdar://problem/3311955> Responder too aggressive at flushing stale data
 Changed mDNSResponder to require four unanswered queries before purging a record, instead of two.

 Revision 1.201  2003/07/04 01:09:41  cheshire
 <rdar://problem/3315775> Need to implement subtype queries
 Modified ConstructServiceName() to allow three-part service types

 Revision 1.200  2003/07/03 23:55:26  cheshire
 Minor change to wording of syslog warning messages

 Revision 1.199  2003/07/03 23:51:13  cheshire
 <rdar://problem/3315652>:	Lots of "have given xxx answers" syslog warnings
 Added more detailed debugging information

 Revision 1.198  2003/07/03 22:19:30  cheshire
 <rdar://problem/3314346> Bug fix in 3274153 breaks TiVo
 Make exception to allow _tivo_servemedia._tcp.

 Revision 1.197  2003/07/02 22:33:05  cheshire
 <rdar://problem/2986146> mDNSResponder needs to start with a smaller cache and then grow it as needed
 Minor refinements:
 When cache is exhausted, verify that rrcache_totalused == rrcache_size and report if not
 Allow cache to grow to 512 records before considering it a potential denial-of-service attack

 Revision 1.196  2003/07/02 21:19:45  cheshire
 <rdar://problem/3313413> Update copyright notices, etc., in source code comments

 Revision 1.195  2003/07/02 19:56:58  cheshire
 <rdar://problem/2986146> mDNSResponder needs to start with a smaller cache and then grow it as needed
 Minor refinement: m->rrcache_active was not being decremented when
 an active record was deleted because its TTL expired

 Revision 1.194  2003/07/02 18:47:40  cheshire
 Minor wording change to log messages

 Revision 1.193  2003/07/02 02:44:13  cheshire
 Fix warning in non-debug build

 Revision 1.192  2003/07/02 02:41:23  cheshire
 <rdar://problem/2986146> mDNSResponder needs to start with a smaller cache and then grow it as needed

 Revision 1.191  2003/07/02 02:30:51  cheshire
 HashSlot() returns an array index. It can't be negative; hence it should not be signed.

 Revision 1.190  2003/06/27 00:03:05  vlubet
 <rdar://problem/3304625> Merge of build failure fix for gcc 3.3

 Revision 1.189  2003/06/11 19:24:03  cheshire
 <rdar://problem/3287141> Crash in SendQueries/SendResponses when no active interfaces
 Slight refinement to previous checkin

 Revision 1.188  2003/06/10 20:33:28  cheshire
 <rdar://problem/3287141> Crash in SendQueries/SendResponses when no active interfaces

 Revision 1.187  2003/06/10 04:30:44  cheshire
 <rdar://problem/3286234> Need to re-probe/re-announce on configuration change
 Only interface-specific records were re-probing and re-announcing, not non-specific records.

 Revision 1.186  2003/06/10 04:24:39  cheshire
 <rdar://problem/3283637> React when we observe other people query unsuccessfully for a record that's in our cache
 Some additional refinements:
 Don't try to do this for unicast-response queries
 better tracking of Qs and KAs in multi-packet KA lists

 Revision 1.185  2003/06/10 03:52:49  cheshire
 Update comments and debug messages

 Revision 1.184  2003/06/10 02:26:39  cheshire
 <rdar://problem/3283516> mDNSResponder needs an mDNS_Reconfirm() function
 Make mDNS_Reconfirm() call mDNS_Lock(), like the other API routines

 Revision 1.183  2003/06/09 18:53:13  cheshire
 Simplify some debugf() statements (replaced block of 25 lines with 2 lines)

 Revision 1.182  2003/06/09 18:38:42  cheshire
 <rdar://problem/3285082> Need to be more tolerant when there are mDNS proxies on the network
 Only issue a correction if the TTL in the proxy packet is less than half the correct value.

 Revision 1.181  2003/06/07 06:45:05  cheshire
 <rdar://problem/3283666> No need for multiple machines to all be sending the same queries

 Revision 1.180  2003/06/07 06:31:07  cheshire
 Create little four-line helper function "FindIdenticalRecordInCache()"

 Revision 1.179  2003/06/07 06:28:13  cheshire
 For clarity, change name of "DNSQuestion q" to "DNSQuestion pktq"

 Revision 1.178  2003/06/07 06:25:12  cheshire
 Update some comments

 Revision 1.177  2003/06/07 04:50:53  cheshire
 <rdar://problem/3283637> React when we observe other people query unsuccessfully for a record that's in our cache

 Revision 1.176  2003/06/07 04:33:26  cheshire
 <rdar://problem/3283540> When query produces zero results, call mDNS_Reconfirm() on any antecedent records
 Minor change: Increment/decrement logic for q->CurrentAnswers should be in
 CacheRecordAdd() and CacheRecordRmv(), not AnswerQuestionWithResourceRecord()

 Revision 1.175  2003/06/07 04:11:52  cheshire
 Minor changes to comments and debug messages

 Revision 1.174  2003/06/07 01:46:38  cheshire
 <rdar://problem/3283540> When query produces zero results, call mDNS_Reconfirm() on any antecedent records

 Revision 1.173  2003/06/07 01:22:13  cheshire
 <rdar://problem/3283516> mDNSResponder needs an mDNS_Reconfirm() function

 Revision 1.172  2003/06/07 00:59:42  cheshire
 <rdar://problem/3283454> Need some randomness to spread queries on the network

 Revision 1.171  2003/06/06 21:41:10  cheshire
 For consistency, mDNS_StopQuery() should return an mStatus result, just like all the other mDNSCore routines

 Revision 1.170  2003/06/06 21:38:55  cheshire
 Renamed 'NewData' as 'FreshData' (The data may not be new data, just a refresh of data that we
 already had in our cache. This refreshes our TTL on the data, but the data itself stays the same.)

 Revision 1.169  2003/06/06 21:35:55  cheshire
 Fix mis-named macro: GetRRHostNameTarget is really GetRRDomainNameTarget
 (the target is a domain name, but not necessarily a host name)

 Revision 1.168  2003/06/06 21:33:31  cheshire
 Instead of using (mDNSPlatformOneSecond/2) all over the place, define a constant "InitialQuestionInterval"

 Revision 1.167  2003/06/06 21:30:42  cheshire
 <rdar://problem/3282962> Don't delay queries for shared record types

 Revision 1.166  2003/06/06 17:20:14  cheshire
 For clarity, rename question fields name/rrtype/rrclass as qname/qtype/qclass
 (Global search-and-replace; no functional change to code execution.)

 Revision 1.165  2003/06/04 02:53:21  cheshire
 Add some "#pragma warning" lines so it compiles clean on Microsoft compilers

 Revision 1.164  2003/06/04 01:25:33  cheshire
 <rdar://problem/3274950> Cannot perform multi-packet known-answer suppression messages
 Display time interval between first and subsequent queries

 Revision 1.163  2003/06/03 19:58:14  cheshire
 <rdar://problem/3277665> mDNS_DeregisterService() fixes:
 When forcibly deregistering after a conflict, ensure we don't send an incorrect goodbye packet.
 Guard against a couple of possible mDNS_DeregisterService() race conditions.

 Revision 1.162  2003/06/03 19:30:39  cheshire
 Minor addition refinements for
 <rdar://problem/3277080> Duplicate registrations not handled as efficiently as they should be

 Revision 1.161  2003/06/03 18:29:03  cheshire
 Minor changes to comments and debugf() messages

 Revision 1.160  2003/06/03 05:02:16  cheshire
 <rdar://problem/3277080> Duplicate registrations not handled as efficiently as they should be

 Revision 1.159  2003/06/03 03:31:57  cheshire
 <rdar://problem/3277033> False self-conflict when there are duplicate registrations on one machine

 Revision 1.158  2003/06/02 22:57:09  cheshire
 Minor clarifying changes to comments and log messages;
 IdenticalResourceRecordAnyInterface() is really more accurately called just IdenticalResourceRecord()

 Revision 1.157  2003/05/31 00:09:49  cheshire
 <rdar://problem/3274862> Add ability to discover what services are on a network

 Revision 1.156  2003/05/30 23:56:49  cheshire
 <rdar://problem/3274847> Crash after error in mDNS_RegisterService()
 Need to set "sr->Extras = mDNSNULL" before returning

 Revision 1.155  2003/05/30 23:48:00  cheshire
 <rdar://problem/3274832> Announcements not properly grouped
 Due to inconsistent setting of rr->LastAPTime at different places in the
 code, announcements were not properly grouped into a single packet.
 Fixed by creating a single routine called InitializeLastAPTime().

 Revision 1.154  2003/05/30 23:38:14  cheshire
 <rdar://problem/3274814> Fix error in IPv6 reverse-mapping PTR records
 Wrote buffer[32] where it should have said buffer[64]

 Revision 1.153  2003/05/30 19:10:56  cheshire
 <rdar://problem/3274153> ConstructServiceName needs to be more restrictive

 Revision 1.152  2003/05/29 22:39:16  cheshire
 <rdar://problem/3273209> Don't truncate strings in the middle of a UTF-8 character

 Revision 1.151  2003/05/29 06:35:42  cheshire
 <rdar://problem/3272221> mDNSCoreReceiveResponse() purging wrong record

 Revision 1.150  2003/05/29 06:25:45  cheshire
 <rdar://problem/3272218> Need to call CheckCacheExpiration() *before* AnswerNewQuestion()

 Revision 1.149  2003/05/29 06:18:39  cheshire
 <rdar://problem/3272217> Split AnswerLocalQuestions into CacheRecordAdd and CacheRecordRmv

 Revision 1.148  2003/05/29 06:11:34  cheshire
 <rdar://problem/3272214> Report if there appear to be too many "Resolve" callbacks

 Revision 1.147  2003/05/29 06:01:18  cheshire
 Change some debugf() calls to LogMsg() calls to help with debugging

 Revision 1.146  2003/05/28 21:00:44  cheshire
 Re-enable "immediate answer burst" debugf message

 Revision 1.145  2003/05/28 20:57:44  cheshire
 <rdar://problem/3271550> mDNSResponder reports "Cannot perform multi-packet
 known-answer suppression ..." This is a known issue caused by a bug in the OS X 10.2
 version of mDNSResponder, so for now we should suppress this warning message.

 Revision 1.144  2003/05/28 18:05:12  cheshire
 <rdar://problem/3009899> mDNSResponder allows invalid service registrations
 Fix silly mistake: old logic allowed "TDP" and "UCP" as valid names

 Revision 1.143  2003/05/28 04:31:29  cheshire
 <rdar://problem/3270733> mDNSResponder not sending probes at the prescribed time

 Revision 1.142  2003/05/28 03:13:07  cheshire
 <rdar://problem/3009899> mDNSResponder allows invalid service registrations
 Require that the transport protocol be _udp or _tcp

 Revision 1.141  2003/05/28 02:19:12  cheshire
 <rdar://problem/3270634> Misleading messages generated by iChat
 Better fix: Only generate the log message for queries where the TC bit is set.

 Revision 1.140  2003/05/28 01:55:24  cheshire
 Minor change to log messages

 Revision 1.139  2003/05/28 01:52:51  cheshire
 <rdar://problem/3270634> Misleading messages generated by iChat

 Revision 1.138  2003/05/27 22:35:00  cheshire
 <rdar://problem/3270277> mDNS_RegisterInterface needs to retrigger questions

 Revision 1.137  2003/05/27 20:04:33  cheshire
 <rdar://problem/3269900> mDNSResponder crash in mDNS_vsnprintf()

 Revision 1.136  2003/05/27 18:50:07  cheshire
 <rdar://problem/3269768> mDNS_StartResolveService doesn't inform client of port number changes

 Revision 1.135  2003/05/26 04:57:28  cheshire
 <rdar://problem/3268953> Delay queries when there are already answers in the cache

 Revision 1.134  2003/05/26 04:54:54  cheshire
 <rdar://problem/3268904> sprintf/vsprintf-style functions are unsafe; use snprintf/vsnprintf instead
 Accidentally deleted '%' case from the switch statement

 Revision 1.133  2003/05/26 03:21:27  cheshire
 Tidy up address structure naming:
 mDNSIPAddr         => mDNSv4Addr (for consistency with mDNSv6Addr)
 mDNSAddr.addr.ipv4 => mDNSAddr.ip.v4
 mDNSAddr.addr.ipv6 => mDNSAddr.ip.v6

 Revision 1.132  2003/05/26 03:01:26  cheshire
 <rdar://problem/3268904> sprintf/vsprintf-style functions are unsafe; use snprintf/vsnprintf instead

 Revision 1.131  2003/05/26 00:42:05  cheshire
 <rdar://problem/3268876> Temporarily include mDNSResponder version in packets

 Revision 1.130  2003/05/24 16:39:48  cheshire
 <rdar://problem/3268631> SendResponses also needs to handle multihoming better

 Revision 1.129  2003/05/23 02:15:37  cheshire
 Fixed misleading use of the term "duplicate suppression" where it should have
 said "known answer suppression". (Duplicate answer suppression is something
 different, and duplicate question suppression is yet another thing, so the use
 of the completely vague term "duplicate suppression" was particularly bad.)

 Revision 1.128  2003/05/23 01:55:13  cheshire
 <rdar://problem/3267127> After name change, mDNSResponder needs to re-probe for name uniqueness

 Revision 1.127  2003/05/23 01:02:15  ksekar
 Bug #: <rdar://problem/3032577>: mDNSResponder needs to include unique id in default name

 Revision 1.126  2003/05/22 02:29:22  cheshire
 <rdar://problem/2984918> SendQueries needs to handle multihoming better
 Complete rewrite of SendQueries. Works much better now :-)

 Revision 1.125  2003/05/22 01:50:45  cheshire
 Fix warnings, and improve log messages

 Revision 1.124  2003/05/22 01:41:50  cheshire
 DiscardDeregistrations doesn't need InterfaceID parameter

 Revision 1.123  2003/05/22 01:38:55  cheshire
 Change bracketing of #pragma mark

 Revision 1.122  2003/05/21 19:59:04  cheshire
 <rdar://problem/3148431> ER: Tweak responder's default name conflict behavior
 Minor refinements; make sure we don't truncate in the middle of a multi-byte UTF-8 character

 Revision 1.121  2003/05/21 17:54:07  ksekar
 Bug #: <rdar://problem/3148431> ER: Tweak responder's default name conflict behavior
 New rename behavior - domain name "foo" becomes "foo--2" on conflict, richtext name becomes "foo (2)"

 Revision 1.120  2003/05/19 22:14:14  ksekar
 <rdar://problem/3162914> mDNS probe denials/conflicts not detected unless conflict is of the same type

 Revision 1.119  2003/05/16 01:34:10  cheshire
 Fix some warnings

 Revision 1.118  2003/05/14 18:48:40  cheshire
 <rdar://problem/3159272> mDNSResponder should be smarter about reconfigurations
 More minor refinements:
 CFSocket.c needs to do *all* its mDNS_DeregisterInterface calls before freeing memory
 mDNS_DeregisterInterface revalidates cache record when *any* representative of an interface goes away

 Revision 1.117  2003/05/14 07:08:36  cheshire
 <rdar://problem/3159272> mDNSResponder should be smarter about reconfigurations
 Previously, when there was any network configuration change, mDNSResponder
 would tear down the entire list of active interfaces and start again.
 That was very disruptive, and caused the entire cache to be flushed,
 and caused lots of extra network traffic. Now it only removes interfaces
 that have really gone, and only adds new ones that weren't there before.

 Revision 1.116  2003/05/14 06:51:56  cheshire
 <rdar://problem/3027144> Rendezvous doesn't refresh server info if changed during sleep

 Revision 1.115  2003/05/14 06:44:31  cheshire
 Improve debugging message

 Revision 1.114  2003/05/07 01:47:03  cheshire
 <rdar://problem/3250330> Also protect against NULL domainlabels

 Revision 1.113  2003/05/07 00:28:18  cheshire
 <rdar://problem/3250330> Need to make mDNSResponder more defensive against bad clients

 Revision 1.112  2003/05/06 00:00:46  cheshire
 <rdar://problem/3248914> Rationalize naming of domainname manipulation functions

 Revision 1.111  2003/05/05 23:42:08  cheshire
 <rdar://problem/3245631> Resolves never succeed
 Was setting "rr->LastAPTime = timenow - rr->LastAPTime"
 instead of  "rr->LastAPTime = timenow - rr->ThisAPInterval"

 Revision 1.110  2003/04/30 21:09:59  cheshire
 <rdar://problem/3244727> mDNS_vsnprintf needs to be more defensive against invalid domain names

 Revision 1.109  2003/04/26 02:41:56  cheshire
 <rdar://problem/3241281> Change timenow from a local variable to a structure member

 Revision 1.108  2003/04/25 01:45:56  cheshire
 <rdar://problem/3240002> mDNS_RegisterNoSuchService needs to include a host name

 Revision 1.107  2003/04/25 00:41:31  cheshire
 <rdar://problem/3239912> Create single routine PurgeCacheResourceRecord(), to avoid bugs in future

 Revision 1.106  2003/04/22 03:14:45  cheshire
 <rdar://problem/3232229> Include Include instrumented mDNSResponder in panther now

 Revision 1.105  2003/04/22 01:07:43  cheshire
 <rdar://problem/3176248> DNSServiceRegistrationUpdateRecord should support a default ttl
 If TTL parameter is zero, leave record TTL unchanged

 Revision 1.104  2003/04/21 19:15:52  cheshire
 Fix some compiler warnings

 Revision 1.103  2003/04/19 02:26:35  cheshire
 Bug #: <rdar://problem/3233804> Incorrect goodbye packet after conflict

 Revision 1.102  2003/04/17 03:06:28  cheshire
 Bug #: <rdar://problem/3231321> No need to query again when a service goes away
 Set UnansweredQueries to 2 when receiving a "goodbye" packet

 Revision 1.101  2003/04/15 20:58:31  jgraessl
 Bug #: 3229014
 Added a hash to lookup records in the cache.

 Revision 1.100  2003/04/15 18:53:14  cheshire
 Bug #: <rdar://problem/3229064> Bug in ScheduleNextTask
 mDNS.c 1.94 incorrectly combined two "if" statements into one.

 Revision 1.99  2003/04/15 18:09:13  jgraessl
 Bug #: 3228892
 Reviewed by: Stuart Cheshire
 Added code to keep track of when the next cache item will expire so we can
 call TidyRRCache only when necessary.

 Revision 1.98  2003/04/03 03:43:55  cheshire
 <rdar://problem/3216837> Off-by-one error in probe rate limiting

 Revision 1.97  2003/04/02 01:48:17  cheshire
 <rdar://problem/3212360> mDNSResponder sometimes suffers false self-conflicts when it sees its own packets
 Additional fix pointed out by Josh:
 Also set ProbeFailTime when incrementing NumFailedProbes when resetting a record back to probing state

 Revision 1.96  2003/04/01 23:58:55  cheshire
 Minor comment changes

 Revision 1.95  2003/04/01 23:46:05  cheshire
 <rdar://problem/3214832> mDNSResponder can get stuck in infinite loop after many location cycles
 mDNS_DeregisterInterface() flushes the RR cache by marking all records received on that interface
 to expire in one second. However, if a mDNS_StartResolveService() call is made in that one-second
 window, it can get an SRV answer from one of those soon-to-be-deleted records, resulting in
 FoundServiceInfoSRV() making an interface-specific query on the interface that was just removed.

 Revision 1.94  2003/03/29 01:55:19  cheshire
 <rdar://problem/3212360> mDNSResponder sometimes suffers false self-conflicts when it sees its own packets
 Solution: Major cleanup of packet timing and conflict handling rules

 Revision 1.93  2003/03/28 01:54:36  cheshire
 Minor tidyup of IPv6 (AAAA) code

 Revision 1.92  2003/03/27 03:30:55  cheshire
 <rdar://problem/3210018> Name conflicts not handled properly, resulting in memory corruption, and eventual crash
 Problem was that HostNameCallback() was calling mDNS_DeregisterInterface(), which is not safe in a callback
 Fixes:
 1. Make mDNS_DeregisterInterface() safe to call from a callback
 2. Make HostNameCallback() use mDNS_DeadvertiseInterface() instead
 (it never really needed to deregister the interface at all)

 Revision 1.91  2003/03/15 04:40:36  cheshire
 Change type called "mDNSOpaqueID" to the more descriptive name "mDNSInterfaceID"

 Revision 1.90  2003/03/14 20:26:37  cheshire
 Reduce debugging messages (reclassify some "debugf" as "verbosedebugf")

 Revision 1.89  2003/03/12 19:57:50  cheshire
 Fixed typo in debug message

 Revision 1.88  2003/03/12 00:17:44  cheshire
 <rdar://problem/3195426> GetFreeCacheRR needs to be more willing to throw away recent records

 Revision 1.87  2003/03/11 01:27:20  cheshire
 Reduce debugging messages (reclassify some "debugf" as "verbosedebugf")

 Revision 1.86  2003/03/06 20:44:33  cheshire
 Comment tidyup

 Revision 1.85  2003/03/05 03:38:35  cheshire
 Bug #: 3185731 Bogus error message in console: died or deallocated, but no record of client can be found!
 Fixed by leaving client in list after conflict, until client explicitly deallocates

 Revision 1.84  2003/03/05 01:27:30  cheshire
 Bug #: 3185482 Different TTL for multicast versus unicast responses
 When building unicast responses, record TTLs are capped to 10 seconds

 Revision 1.83  2003/03/04 23:48:52  cheshire
 Bug #: 3188865 Double probes after wake from sleep
 Don't reset record type to kDNSRecordTypeUnique if record is DependentOn another

 Revision 1.82  2003/03/04 23:38:29  cheshire
 Bug #: 3099194 mDNSResponder needs performance improvements
 Only set rr->CRActiveQuestion to point to the
 currently active representative of a question set

 Revision 1.81  2003/02/21 03:35:34  cheshire
 Bug #: 3179007 mDNSResponder needs to include AAAA records in additional answer section

 Revision 1.80  2003/02/21 02:47:53  cheshire
 Bug #: 3099194 mDNSResponder needs performance improvements
 Several places in the code were calling CacheRRActive(), which searched the entire
 question list every time, to see if this cache resource record answers any question.
 Instead, we now have a field "CRActiveQuestion" in the resource record structure

 Revision 1.79  2003/02/21 01:54:07  cheshire
 Bug #: 3099194 mDNSResponder needs performance improvements
 Switched to using new "mDNS_Execute" model (see "Implementer Notes.txt")

 Revision 1.78  2003/02/20 06:48:32  cheshire
 Bug #: 3169535 Xserve RAID needs to do interface-specific registrations
 Reviewed by: Josh Graessley, Bob Bradley

 Revision 1.77  2003/01/31 03:35:59  cheshire
 Bug #: 3147097 mDNSResponder sometimes fails to find the correct results
 When there were *two* active questions in the list, they were incorrectly
 finding *each other* and *both* being marked as duplicates of another question

 Revision 1.76  2003/01/29 02:46:37  cheshire
 Fix for IPv6:
 A physical interface is identified solely by its InterfaceID (not by IP and type).
 On a given InterfaceID, mDNSCore may send both v4 and v6 multicasts.
 In cases where the requested outbound protocol (v4 or v6) is not supported on
 that InterfaceID, the platform support layer should simply discard that packet.

 Revision 1.75  2003/01/29 01:47:40  cheshire
 Rename 'Active' to 'CRActive' or 'InterfaceActive' for improved clarity

 Revision 1.74  2003/01/28 05:26:25  cheshire
 Bug #: 3147097 mDNSResponder sometimes fails to find the correct results
 Add 'Active' flag for interfaces

 Revision 1.73  2003/01/28 03:45:12  cheshire
 Fixed missing "not" in "!mDNSAddrIsDNSMulticast(dstaddr)"

 Revision 1.72  2003/01/28 01:49:48  cheshire
 Bug #: 3147097 mDNSResponder sometimes fails to find the correct results
 FindDuplicateQuestion() was incorrectly finding the question itself in the list,
 and incorrectly marking it as a duplicate (of itself), so that it became inactive.

 Revision 1.71  2003/01/28 01:41:44  cheshire
 Bug #: 3153091 Race condition when network change causes bad stuff
 When an interface goes away, interface-specific questions on that interface become orphaned.
 Orphan questions cause HaveQueries to return true, but there's no interface to send them on.
 Fix: mDNS_DeregisterInterface() now calls DeActivateInterfaceQuestions()

 Revision 1.70  2003/01/23 19:00:20  cheshire
 Protect against infinite loops in mDNS_Execute

 Revision 1.69  2003/01/21 22:56:32  jgraessl
 Bug #: 3124348  service name changes are not properly handled
 Submitted by: Stuart Cheshire
 Reviewed by: Joshua Graessley
 Applying changes for 3124348 to main branch. 3124348 changes went in to a
 branch for SU.

 Revision 1.68  2003/01/17 04:09:27  cheshire
 Bug #: 3141038 mDNSResponder Resolves are unreliable on multi-homed hosts

 Revision 1.67  2003/01/17 03:56:45  cheshire
 Default 24-hour TTL is far too long. Changing to two hours.

 Revision 1.66  2003/01/13 23:49:41  jgraessl
 Merged changes for the following fixes in to top of tree:
 3086540  computer name changes not handled properly
 3124348  service name changes are not properly handled
 3124352  announcements sent in pairs, failing chattiness test

 Revision 1.65  2002/12/23 22:13:28  jgraessl
 Reviewed by: Stuart Cheshire
 Initial IPv6 support for mDNSResponder.

 Revision 1.64  2002/11/26 20:49:06  cheshire
 Bug #: 3104543 RFC 1123 allows the first character of a name label to be either a letter or a digit

 Revision 1.63  2002/09/21 20:44:49  zarzycki
 Added APSL info

 Revision 1.62  2002/09/20 03:25:37  cheshire
 Fix some compiler warnings

 Revision 1.61  2002/09/20 01:05:24  cheshire
 Don't kill the Extras list in mDNS_DeregisterService()

 Revision 1.60  2002/09/19 23:47:35  cheshire
 Added mDNS_RegisterNoSuchService() function for assertion of non-existance
 of a particular named service

 Revision 1.59  2002/09/19 21:25:34  cheshire
 mDNS_snprintf() doesn't need to be in a separate file

 Revision 1.58  2002/09/19 04:20:43  cheshire
 Remove high-ascii characters that confuse some systems

 Revision 1.57  2002/09/17 01:07:08  cheshire
 Change mDNS_AdvertiseLocalAddresses to be a parameter to mDNS_Init()

 Revision 1.56  2002/09/16 19:44:17  cheshire
 Merge in license terms from Quinn's copy, in preparation for Darwin release
*/

#define TEST_LOCALONLY_FOR_EVERYTHING 0
#include "wm_mem.h"
#include "mDNSClientAPI.h"				// Defines the interface provided to the client layer above
#include "mDNSPlatformFunctions.h"		// Defines the interface required of the supporting layer below

// Disable certain benign warnings with Microsoft compilers
#if(defined(_MSC_VER))
// Disable "conditional expression is constant" warning for debug macros.
// Otherwise, this generates warnings for the perfectly natural construct "while(1)"
// If someone knows a variant way of writing "while(1)" that doesn't generate warning messages, please let us know
#pragma warning(disable:4127)
	
// Disable "const object should be initialized"
// We know that static/globals are defined to be zeroed in ANSI C, and to avoid this warning would require some
// *really* ugly chunk of zeroes and curly braces to initialize zeroRR and mDNSprintf_format_default to all zeroes
#pragma warning(disable:4132)
	
// Disable "assignment within conditional expression".
// Other compilers understand the convention that if you place the assignment expression within an extra pair
// of parentheses, this signals to the compiler that you really intended an assignment and no warning is necessary.
// The Microsoft compiler doesn't understand this convention, so in the absense of any other way to signal
// to the compiler that the assignment is intentional, we have to just turn this warning off completely.
#pragma warning(disable:4706)
#endif

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark - DNS Protocol Constants
#endif

typedef enum
{
	kDNSFlag0_QR_Mask     = 0x80,		// Query or response?
	kDNSFlag0_QR_Query    = 0x00,
	kDNSFlag0_QR_Response = 0x80,
	
	kDNSFlag0_OP_Mask     = 0x78,		// Operation type
	kDNSFlag0_OP_StdQuery = 0x00,
	kDNSFlag0_OP_Iquery   = 0x08,
	kDNSFlag0_OP_Status   = 0x10,
	kDNSFlag0_OP_Unused3  = 0x18,
	kDNSFlag0_OP_Notify   = 0x20,
	kDNSFlag0_OP_Update   = 0x28,
	
	kDNSFlag0_QROP_Mask   = kDNSFlag0_QR_Mask | kDNSFlag0_OP_Mask,
	
	kDNSFlag0_AA          = 0x04,		// Authoritative Answer?
	kDNSFlag0_TC          = 0x02,		// Truncated?
	kDNSFlag0_RD          = 0x01,		// Recursion Desired?
	kDNSFlag1_RA          = 0x80,		// Recursion Available?
	
	kDNSFlag1_Zero        = 0x40,		// Reserved; must be zero
	kDNSFlag1_AD          = 0x20,		// Authentic Data [RFC 2535]
	kDNSFlag1_CD          = 0x10,		// Checking Disabled [RFC 2535]

	kDNSFlag1_RC          = 0x0F,		// Response code
	kDNSFlag1_RC_NoErr    = 0x00,
	kDNSFlag1_RC_FmtErr   = 0x01,
	kDNSFlag1_RC_SrvErr   = 0x02,
	kDNSFlag1_RC_NXDomain = 0x03,
	kDNSFlag1_RC_NotImpl  = 0x04,
	kDNSFlag1_RC_Refused  = 0x05,
	kDNSFlag1_RC_YXDomain = 0x06,
	kDNSFlag1_RC_YXRRSet  = 0x07,
	kDNSFlag1_RC_NXRRSet  = 0x08,
	kDNSFlag1_RC_NotAuth  = 0x09,
	kDNSFlag1_RC_NotZone  = 0x0A
} DNS_Flags;

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Program Constants
#endif

mDNSexport const ResourceRecord  zeroRR;
mDNSexport const mDNSIPPort      zeroIPPort        = { { 0 } };
mDNSexport const mDNSv4Addr      zeroIPAddr        = { { 0 } };
mDNSexport const mDNSv6Addr      zerov6Addr        = { { 0 } };
mDNSexport const mDNSv4Addr      onesIPv4Addr      = { { 255, 255, 255, 255 } };
mDNSexport const mDNSv6Addr      onesIPv6Addr      = { { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 } };
//mDNSlocal  const mDNSAddr        zeroAddr          = { mDNSAddrType_None, {{{ 0 }}} };

mDNSexport const mDNSInterfaceID mDNSInterface_Any = { 0 };
mDNSlocal  const mDNSInterfaceID mDNSInterfaceMark = { (mDNSInterfaceID)~0 };

#define UnicastDNSPortAsNumber 53
#define MulticastDNSPortAsNumber 5353
mDNSexport const mDNSIPPort UnicastDNSPort     = { { UnicastDNSPortAsNumber   >> 8, UnicastDNSPortAsNumber   & 0xFF } };
mDNSexport const mDNSIPPort MulticastDNSPort   = { { MulticastDNSPortAsNumber >> 8, MulticastDNSPortAsNumber & 0xFF } };
mDNSexport const mDNSv4Addr AllDNSAdminGroup   = { { 239, 255, 255, 251 } };
mDNSexport const mDNSv4Addr AllDNSLinkGroup    = { { 224,   0,   0, 251 } };
mDNSexport const mDNSv6Addr AllDNSLinkGroupv6  = { { 0xFF,0x02,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0xFB } };
mDNSexport const mDNSAddr   AllDNSLinkGroup_v4 = { mDNSAddrType_IPv4, { { { 224,   0,   0, 251 } } } };
mDNSexport const mDNSAddr   AllDNSLinkGroup_v6 = { mDNSAddrType_IPv6, { { { 0xFF,0x02,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0xFB } } } };

static const mDNSOpaque16 zeroID = { { 0, 0 } };
static const mDNSOpaque16 QueryFlags    = { { kDNSFlag0_QR_Query    | kDNSFlag0_OP_StdQuery,                0 } };
static const mDNSOpaque16 ResponseFlags = { { kDNSFlag0_QR_Response | kDNSFlag0_OP_StdQuery | kDNSFlag0_AA, 0 } };
#define zeroDomainNamePtr ((domainname*)"")

// Any records bigger than this are considered 'large' records
#define SmallRecordLimit 1024

#define kDefaultTTLforUnique 240
#define kDefaultTTLforShared (2*3600)

#define kMaxUpdateCredits 10

#define kNextScheduledTime   0x14000
/*
static const char *const mDNS_DomainTypeNames[] =
{
	"_browse._dns-sd._udp.local.",
	"_default._browse._dns-sd._udp.local.",
	"_register._dns-sd._udp.local.",
	"_default._register._dns-sd._udp.local."
};
*/
#define AssignDomainName(DST, SRC) mDNSPlatformMemCopy((SRC).c, (DST).c, DomainNameLength(&(SRC)))

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Specialized mDNS version of vsnprintf
#endif

#include <stdio.h>
#include <string.h>
//#include <asm/types.h>

static const struct mDNSprintf_format
{
	unsigned      leftJustify : 1;
	unsigned      forceSign : 1;
	unsigned      zeroPad : 1;
	unsigned      havePrecision : 1;
	unsigned      hSize : 1;
	unsigned      lSize : 1;
	char          altForm;
	char          sign;		// +, - or space
	unsigned int  fieldWidth;
	unsigned int  precision;
} mDNSprintf_format_default;

mDNSexport mDNSu32 mDNS_vsnprintf(char *sbuffer, mDNSu32 buflen, const char *fmt, va_list arg)
{
	mDNSu32 nwritten = 0;
	int c;
	buflen--;		// Pre-reserve one space in the buffer for the terminating nul
	#define mDNS_VACB_Size 300
	char *mDNS_VACB = tls_mem_alloc(mDNS_VACB_Size);
	if(!mDNS_VACB) goto exit;
	memset(mDNS_VACB, 0, mDNS_VACB_Size);
	for (c = *fmt; c != 0; c = *++fmt)
	{
		if (c != '%')
		{
			*sbuffer++ = (char)c;
			if (++nwritten >= buflen) goto exit;
		}
		else
		{
			unsigned int i=0, j;
			// The mDNS Vsprintf Argument Conversion Buffer is used as a temporary holding area for
			// generating decimal numbers, hexdecimal numbers, IP addresses, domain name strings, etc.
			// The size needs to be enough for a 256-byte domain name plus some error text.

#define mDNS_VACB_Lim (&mDNS_VACB[mDNS_VACB_Size])
#define mDNS_VACB_Remain(s) ((mDNSu32)(mDNS_VACB_Lim - s))
			char *s = mDNS_VACB_Lim, *digits;
			struct mDNSprintf_format F = mDNSprintf_format_default;
	
			while (1)	//  decode flags
			{
				c = *++fmt;
				if      (c == '-') F.leftJustify = 1;
				else if (c == '+') F.forceSign = 1;
				else if (c == ' ') F.sign = ' ';
				else if (c == '#') F.altForm++;
				else if (c == '0') F.zeroPad = 1;
				else break;
			}
	
			if (c == '*')	//  decode field width
			{
				int f = va_arg(arg, int);
				if (f < 0) { f = -f; F.leftJustify = 1; }
				F.fieldWidth = (unsigned int)f;
				c = *++fmt;
			}
			else
			{
				for (; c >= '0' && c <= '9'; c = *++fmt)
					F.fieldWidth = (10 * F.fieldWidth) + (c - '0');
			}
	
			if (c == '.')	//  decode precision
			{
				if ((c = *++fmt) == '*')
				{ F.precision = va_arg(arg, unsigned int); c = *++fmt; }
				else for (; c >= '0' && c <= '9'; c = *++fmt)
					F.precision = (10 * F.precision) + (c - '0');
				F.havePrecision = 1;
			}
	
			if (F.leftJustify) F.zeroPad = 0;
	
		conv:
			switch (c)	//  perform appropriate conversion
			{
				unsigned long n;
			case 'h' :	F.hSize = 1; c = *++fmt; goto conv;
			case 'l' :	// fall through
			case 'L' :	F.lSize = 1; c = *++fmt; goto conv;
			case 'd' :
			case 'i' :	if (F.lSize) n = (unsigned long)va_arg(arg, long);
			else n = (unsigned long)va_arg(arg, int);
				if (F.hSize) n = (short) n;
				if ((long) n < 0) { n = (unsigned long)-(long)n; F.sign = '-'; }
				else if (F.forceSign) F.sign = '+';
				goto decimal;
			case 'u' :	if (F.lSize) n = va_arg(arg, unsigned long);
			else n = va_arg(arg, unsigned int);
				if (F.hSize) n = (unsigned short) n;
				F.sign = 0;
				goto decimal;
			decimal:	if (!F.havePrecision)
			{
				if (F.zeroPad)
				{
					F.precision = F.fieldWidth;
					if (F.sign) --F.precision;
				}
				if (F.precision < 1) F.precision = 1;
			}
				if (F.precision > mDNS_VACB_Size - 1)
					F.precision = mDNS_VACB_Size - 1;
				for (i = 0; n; n /= 10, i++) *--s = (char)(n % 10 + '0');
				for (; i < F.precision; i++) *--s = '0';
				if (F.sign) { *--s = F.sign; i++; }
				break;
	
			case 'o' :	if (F.lSize) n = va_arg(arg, unsigned long);
			else n = va_arg(arg, unsigned int);
				if (F.hSize) n = (unsigned short) n;
				if (!F.havePrecision)
				{
					if (F.zeroPad) F.precision = F.fieldWidth;
					if (F.precision < 1) F.precision = 1;
				}
				if (F.precision > mDNS_VACB_Size - 1)
					F.precision = mDNS_VACB_Size - 1;
				for (i = 0; n; n /= 8, i++) *--s = (char)(n % 8 + '0');
				if (F.altForm && i && *s != '0') { *--s = '0'; i++; }
				for (; i < F.precision; i++) *--s = '0';
				break;
	
			case 'a' :	{
				unsigned char *a = va_arg(arg, unsigned char *);
				if (!a) { static char emsg[] = "<<NULL>>"; s = emsg; i = sizeof(emsg)-1; }
				else
				{
					unsigned short *w = (unsigned short *)a;
					s = mDNS_VACB;	// Adjust s to point to the start of the buffer, not the end
					if (F.altForm)
					{
						mDNSAddr *ip = (mDNSAddr*)a;
						a = (unsigned char  *)&ip->ip.v4;
						w = (unsigned short *)&ip->ip.v6;
						switch (ip->type)
						{
						case mDNSAddrType_IPv4: F.precision =  4; break;
						case mDNSAddrType_IPv6: F.precision = 16; break;
						default:                F.precision =  0; break;
						}
					}
					switch (F.precision)
					{
					case  4: i = mDNS_snprintf(mDNS_VACB, mDNS_VACB_Size, "%d.%d.%d.%d",
								   a[0], a[1], a[2], a[3]); break;
					case  6: i = mDNS_snprintf(mDNS_VACB, mDNS_VACB_Size, "%02X:%02X:%02X:%02X:%02X:%02X",
								   a[0], a[1], a[2], a[3], a[4], a[5]); break;
					case 16: i = mDNS_snprintf(mDNS_VACB, mDNS_VACB_Size, "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
								   w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7]); break;
					default: i = mDNS_snprintf(mDNS_VACB, mDNS_VACB_Size, "%s", "<< ERROR: Must specify address size "
								   "(i.e. %.4a=IPv4, %.6a=Ethernet, %.16a=IPv6) >>"); break;
					}
				}
			}
				break;
	
			case 'p' :	F.havePrecision = F.lSize = 1;
				F.precision = 8;
			case 'X' :	digits = "0123456789ABCDEF";
				goto hexadecimal;
			case 'x' :	digits = "0123456789abcdef";
			hexadecimal:if (F.lSize) n = va_arg(arg, unsigned long);
			else n = va_arg(arg, unsigned int);
			if (F.hSize) n = (unsigned short) n;
			if (!F.havePrecision)
			{
				if (F.zeroPad)
				{
					F.precision = F.fieldWidth;
					if (F.altForm) F.precision -= 2;
				}
				if (F.precision < 1) F.precision = 1;
			}
			if (F.precision > mDNS_VACB_Size - 1)
				F.precision = mDNS_VACB_Size - 1;
			for (i = 0; n; n /= 16, i++) *--s = digits[n % 16];
			for (; i < F.precision; i++) *--s = '0';
			if (F.altForm) { *--s = (char)c; *--s = '0'; i += 2; }
			break;
	
			case 'c' :	*--s = (char)va_arg(arg, int); i = 1; break;
	
			case 's' :	s = va_arg(arg, char *);
				if (!s) { static char emsg[] = "<<NULL>>"; s = emsg; i = sizeof(emsg)-1; }
				else switch (F.altForm)
				{
				case 0: { char *a=s; i=0; while(*a++) i++; break; }	// C string
				case 1: i = (unsigned char) *s++; break;	// Pascal string
				case 2: {									// DNS label-sequence name
					unsigned char *a = (unsigned char *)s;
					s = mDNS_VACB;	// Adjust s to point to the start of the buffer, not the end
					if (*a == 0) *s++ = '.';	// Special case for root DNS name
					while (*a)
					{
						if (*a > 63) { s += mDNS_snprintf(s, mDNS_VACB_Remain(s), "<<INVALID LABEL LENGTH %u>>", *a); break; }
						if (s + *a >= &mDNS_VACB[254]) { s += mDNS_snprintf(s, mDNS_VACB_Remain(s), "<<NAME TOO LONG>>"); break; }
						s += mDNS_snprintf(s, mDNS_VACB_Remain(s), "%#s.", a);
						a += 1 + *a;
					}
					i = (mDNSu32)(s - mDNS_VACB);
					s = mDNS_VACB;	// Reset s back to the start of the buffer
					break;
				}
				}
				if (F.havePrecision && i > F.precision)		// Make sure we don't truncate in the middle of a UTF-8 character
				{ i = F.precision; while (i>0 && (s[i] & 0xC0) == 0x80) i--; }
				break;
	
			case 'n' :	s = va_arg(arg, char *);
				if      (F.hSize) * (short *) s = (short)nwritten;
				else if (F.lSize) * (long  *) s = (long)nwritten;
				else              * (int   *) s = (int)nwritten;
				continue;
	
			default:	s = mDNS_VACB;
				i = mDNS_snprintf(mDNS_VACB, mDNS_VACB_Size, "<<UNKNOWN FORMAT CONVERSION CODE %%%c>>", c);

			case '%' :	*sbuffer++ = (char)c;
				if (++nwritten >= buflen) goto exit;
				break;
			}
	
			if (i < F.fieldWidth && !F.leftJustify)			// Pad on the left
				do	{
					*sbuffer++ = ' ';
					if (++nwritten >= buflen) goto exit;
				} while (i < --F.fieldWidth);
	
			if (i > buflen - nwritten)	// Make sure we don't truncate in the middle of a UTF-8 character
			{ i = buflen - nwritten; while (i>0 && (s[i] & 0xC0) == 0x80) i--; }
			for (j=0; j<i; j++) *sbuffer++ = *s++;			// Write the converted result
			nwritten += i;
			if (nwritten >= buflen) goto exit;
	
			for (; i < F.fieldWidth; i++)					// Pad on the right
			{
				*sbuffer++ = ' ';
				if (++nwritten >= buflen) goto exit;
			}
		}
	}
 exit:
 	if(mDNS_VACB)
		tls_mem_free(mDNS_VACB);
	*sbuffer++ = 0;
	return(nwritten);
}

mDNSexport mDNSu32 mDNS_snprintf(char *sbuffer, mDNSu32 buflen, const char *fmt, ...)
{
	mDNSu32 length;
	
	va_list ptr;
	va_start(ptr,fmt);
	length = mDNS_vsnprintf(sbuffer, buflen, fmt, ptr);
	va_end(ptr);
	
	return(length);
}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - General Utility Functions
#endif

mDNSexport char *DNSTypeName(mDNSu16 rrtype)
{
	switch (rrtype)
	{
	case kDNSType_A:    return("Addr");
	case kDNSType_CNAME:return("CNAME");
	case kDNSType_NULL: return("NULL");
	case kDNSType_PTR:  return("PTR");
	case kDNSType_HINFO:return("HINFO");
	case kDNSType_TXT:  return("TXT");
	case kDNSType_AAAA: return("AAAA");
	case kDNSType_SRV:  return("SRV");
	case kDNSQType_ANY: return("ANY");
	default:			{
		static char buffer[16];
		mDNS_snprintf(buffer, sizeof(buffer), "(%d)", rrtype);
		return(buffer);
	}
	}
}

mDNSexport char *GetRRDisplayString_rdb(mDNS *const m, const ResourceRecord *rr, RDataBody *rd)
{
	char *ptr = m->MsgBuffer;
	mDNSu32 length = mDNS_snprintf(m->MsgBuffer, 79, "%4d %##s %s ", rr->rdlength, rr->name.c, DNSTypeName(rr->rrtype));
	switch (rr->rrtype)
	{
	case kDNSType_A:	mDNS_snprintf(m->MsgBuffer+length, 79-length, "%.4a", &rd->ip);         break;
	case kDNSType_CNAME:// Same as PTR
	case kDNSType_PTR:	mDNS_snprintf(m->MsgBuffer+length, 79-length, "%##s", &rd->name);       break;
	case kDNSType_HINFO:// Display this the same as TXT (just show first string)
	case kDNSType_TXT:	mDNS_snprintf(m->MsgBuffer+length, 79-length, "%#s", rd->txt.c);	break;
	case kDNSType_AAAA:	mDNS_snprintf(m->MsgBuffer+length, 79-length, "%.16a", &rd->ipv6);      break;
	case kDNSType_SRV:	mDNS_snprintf(m->MsgBuffer+length, 79-length, "%##s", &rd->srv.target); break;
	default:			mDNS_snprintf(m->MsgBuffer+length, 79-length, "RDLen %d: %s",
						      rr->rdlength, rd->data);  break;
	}
	for (ptr = m->MsgBuffer; *ptr; ptr++) if (*ptr < ' ') *ptr='.';
	return(m->MsgBuffer);
}

mDNSlocal mDNSu32 mDNSRandom(mDNSu32 max)
{
	static mDNSu32 seed = 0;
	mDNSu32 mask = 1;
	
	if (!seed) seed = (mDNSu32)mDNSPlatformTimeNow();
	while (mask < max) mask = (mask << 1) | 1;
	do seed = seed * 21 + 1; while ((seed & mask) > max);
	return (seed & mask);
}

#define mDNSSameIPv4Address(A,B) ((A).NotAnInteger == (B).NotAnInteger)
#define mDNSSameIPv6Address(A,B) ((A).l[0] == (B).l[0] && (A).l[1] == (B).l[1] && (A).l[2] == (B).l[2] && (A).l[3] == (B).l[3])

#define mDNSIPv4AddressIsZero(A) mDNSSameIPv4Address((A), zeroIPAddr)
#define mDNSIPv6AddressIsZero(A) mDNSSameIPv6Address((A), zerov6Addr)

#define mDNSIPv4AddressIsOnes(A) mDNSSameIPv4Address((A), onesIPv4Addr)
#define mDNSIPv6AddressIsOnes(A) mDNSSameIPv6Address((A), onesIPv6Addr)

#define mDNSAddressIsZero(X) (                                              \
	((X)->type == mDNSAddrType_IPv4 && mDNSIPv4AddressIsZero((X)->ip.v4)) || \
	((X)->type == mDNSAddrType_IPv6 && mDNSIPv6AddressIsZero((X)->ip.v6))    )

#define mDNSAddressIsOnes(X) (                                              \
	((X)->type == mDNSAddrType_IPv4 && mDNSIPv4AddressIsOnes((X)->ip.v4)) || \
	((X)->type == mDNSAddrType_IPv6 && mDNSIPv6AddressIsOnes((X)->ip.v6))    )

#define mDNSAddressIsValid(X) (                                                                                             \
	((X)->type == mDNSAddrType_IPv4) ? !(mDNSIPv4AddressIsZero((X)->ip.v4) || mDNSIPv4AddressIsOnes((X)->ip.v4)) :          \
	((X)->type == mDNSAddrType_IPv6) ? !(mDNSIPv6AddressIsZero((X)->ip.v6) || mDNSIPv6AddressIsOnes((X)->ip.v6)) : mDNSfalse)

mDNSexport mDNSBool mDNSSameAddress(const mDNSAddr *ip1, const mDNSAddr *ip2)
{
	if (ip1->type == ip2->type)
	{
		switch (ip1->type)
		{
		case mDNSAddrType_IPv4 : return(mDNSBool)(mDNSSameIPv4Address(ip1->ip.v4, ip2->ip.v4));
		case mDNSAddrType_IPv6 : return(mDNSBool)(mDNSSameIPv6Address(ip1->ip.v6, ip2->ip.v6));
		}
	}
	return(mDNSfalse);
}

mDNSexport mDNSBool mDNSAddrIsDNSMulticast(const mDNSAddr *ip)
{
	switch(ip->type)
	{
	case mDNSAddrType_IPv4: return(mDNSBool)(ip->ip.v4.NotAnInteger == AllDNSLinkGroup.NotAnInteger);
	case mDNSAddrType_IPv6: return(mDNSBool)(ip->ip.v6.l[0] == AllDNSLinkGroupv6.l[0] &&
						 ip->ip.v6.l[1] == AllDNSLinkGroupv6.l[1] &&
						 ip->ip.v6.l[2] == AllDNSLinkGroupv6.l[2] &&
						 ip->ip.v6.l[3] == AllDNSLinkGroupv6.l[3] );
	default: return(mDNSfalse);
	}
}

mDNSlocal const NetworkInterfaceInfo *GetFirstActiveInterface(const NetworkInterfaceInfo *intf)
{
	while (intf && !intf->InterfaceActive) intf = intf->next;
	return(intf);
}

mDNSlocal mDNSInterfaceID GetNextActiveInterfaceID(const NetworkInterfaceInfo *intf)
{
	const NetworkInterfaceInfo *next = GetFirstActiveInterface(intf->next);
	if (next) return(next->InterfaceID); else return(mDNSNULL);
}

#define InitialQuestionInterval (mDNSPlatformOneSecond/2)
#define ActiveQuestion(Q) ((Q)->ThisQInterval > 0 && !(Q)->DuplicateOf)
#define TimeToSendThisQuestion(Q,time) (ActiveQuestion(Q) && (time) - ((Q)->LastQTime + (Q)->ThisQInterval) >= 0)

mDNSlocal void SetNextQueryTime(mDNS *const m, const DNSQuestion *const q)
{
	if (ActiveQuestion(q))
		if (m->NextScheduledQuery - (q->LastQTime + q->ThisQInterval) > 0)
			m->NextScheduledQuery = (q->LastQTime + q->ThisQInterval);
}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Domain Name Utility Functions
#endif

#define mdnsIsDigit(X)     ((X) >= '0' && (X) <= '9')
#define mDNSIsUpperCase(X) ((X) >= 'A' && (X) <= 'Z')
#define mDNSIsLowerCase(X) ((X) >= 'a' && (X) <= 'z')
#define mdnsIsLetter(X)    (mDNSIsUpperCase(X) || mDNSIsLowerCase(X))

mDNSexport mDNSBool SameDomainLabel(const mDNSu8 *a, const mDNSu8 *b)
{
	int i;
	const int len = *a++;

	if (len > MAX_DOMAIN_LABEL)
	{ debugf("Malformed label (too long)"); return(mDNSfalse); }

	if (len != *b++) return(mDNSfalse);
	for (i=0; i<len; i++)
	{
		mDNSu8 ac = *a++;
		mDNSu8 bc = *b++;
		if (mDNSIsUpperCase(ac)) ac += 'a' - 'A';
		if (mDNSIsUpperCase(bc)) bc += 'a' - 'A';
		if (ac != bc) return(mDNSfalse);
	}
	return(mDNStrue);
}

mDNSexport mDNSBool SameDomainName(const domainname *const d1, const domainname *const d2)
{
	const mDNSu8 *      a   = d1->c;
	const mDNSu8 *      b   = d2->c;
	const mDNSu8 *const max = d1->c + MAX_DOMAIN_NAME;			// Maximum that's valid

	while (*a || *b)
	{
		if (a + 1 + *a >= max)
		{ debugf("Malformed domain name (more than 255 characters)"); return(mDNSfalse); }
		if (!SameDomainLabel(a, b)) return(mDNSfalse);
		a += 1 + *a;
		b += 1 + *b;
	}
	
	return(mDNStrue);
}

// Returns length of a domain name INCLUDING the byte for the final null label
// i.e. for the root label "." it returns one
// For the FQDN "com." it returns 5 (length byte, three data bytes, final zero)
// Legal results are 1 (just root label) to 255 (MAX_DOMAIN_NAME)
// If the given domainname is invalid, result is 256
mDNSexport mDNSu16 DomainNameLength(const domainname *const name)
{
	const mDNSu8 *src = name->c;
	while (*src)
	{
		if (*src > MAX_DOMAIN_LABEL) return(MAX_DOMAIN_NAME+1);
		src += 1 + *src;
		if (src - name->c >= MAX_DOMAIN_NAME) return(MAX_DOMAIN_NAME+1);
	}
	return((mDNSu16)(src - name->c + 1));
}

// CompressedDomainNameLength returns the length of a domain name INCLUDING the byte
// for the final null label i.e. for the root label "." it returns one.
// E.g. for the FQDN "foo.com." it returns 9
// (length, three data bytes, length, three more data bytes, final zero).
// In the case where a parent domain name is provided, and the given name is a child
// of that parent, CompressedDomainNameLength returns the length of the prefix portion
// of the child name, plus TWO bytes for the compression pointer.
// E.g. for the name "foo.com." with parent "com.", it returns 6
// (length, three data bytes, two-byte compression pointer).
mDNSlocal mDNSu16 CompressedDomainNameLength(const domainname *const name, const domainname *parent)
{
	const mDNSu8 *src = name->c;
	if (parent && parent->c[0] == 0) parent = mDNSNULL;
	while (*src)
	{
		if (*src > MAX_DOMAIN_LABEL) return(MAX_DOMAIN_NAME+1);
		if (parent && SameDomainName((domainname *)src, parent)) return((mDNSu16)(src - name->c + 2));
		//printf("%.*s\n", *src, src+1);
		src += 1 + *src;
		if (src - name->c >= MAX_DOMAIN_NAME) return(MAX_DOMAIN_NAME+1);
	}
	return((mDNSu16)(src - name->c + 1));
}

// AppendLiteralLabelString appends a single label to an existing (possibly empty) domainname.
// The C string contains the label as-is, with no escaping, etc.
// Any dots in the name are literal dots, not label separators
// If successful, AppendLiteralLabelString returns a pointer to the next unused byte
// in the domainname bufer (i.e., the next byte after the terminating zero).
// If unable to construct a legal domain name (i.e. label more than 63 bytes, or total more than 255 bytes)
// AppendLiteralLabelString returns mDNSNULL.
mDNSexport mDNSu8 *AppendLiteralLabelString(domainname *const name, const char *cstr)
{
	mDNSu8       *      ptr  = name->c + DomainNameLength(name) - 1;	// Find end of current name
	const mDNSu8 *const lim1 = name->c + MAX_DOMAIN_NAME - 1;			// Limit of how much we can add (not counting final zero)
	const mDNSu8 *const lim2 = ptr + 1 + MAX_DOMAIN_LABEL;
	const mDNSu8 *const lim  = (lim1 < lim2) ? lim1 : lim2;
	mDNSu8       *lengthbyte = ptr++;									// Record where the length is going to go
	
	while (*cstr && ptr < lim) *ptr++ = (mDNSu8)*cstr++;	// Copy the data
	*lengthbyte = (mDNSu8)(ptr - lengthbyte - 1);			// Fill in the length byte
	*ptr++ = 0;												// Put the null root label on the end
	if (*cstr) return(mDNSNULL);							// Failure: We didn't successfully consume all input
	else return(ptr);										// Success: return new value of ptr
}

// AppendDNSNameString appends zero or more labels to an existing (possibly empty) domainname.
// The C string is in conventional DNS syntax:
// Textual labels, escaped as necessary using the usual DNS '\' notation, separated by dots.
// If successful, AppendDNSNameString returns a pointer to the next unused byte
// in the domainname bufer (i.e., the next byte after the terminating zero).
// If unable to construct a legal domain name (i.e. label more than 63 bytes, or total more than 255 bytes)
// AppendDNSNameString returns mDNSNULL.
mDNSexport mDNSu8 *AppendDNSNameString(domainname *const name, const char *cstr)
{
	mDNSu8       *      ptr = name->c + DomainNameLength(name) - 1;	// Find end of current name
	const mDNSu8 *const lim = name->c + MAX_DOMAIN_NAME - 1;		// Limit of how much we can add (not counting final zero)
	while (*cstr && ptr < lim)										// While more characters, and space to put them...
	{
		mDNSu8 *lengthbyte = ptr++;									// Record where the length is going to go
		while (*cstr && *cstr != '.' && ptr < lim)					// While we have characters in the label...
		{
			mDNSu8 c = (mDNSu8)*cstr++;								// Read the character
			if (c == '\\')											// If escape character, check next character
			{
				if (*cstr == '\\' || *cstr == '.')					// If a second escape, or a dot,
					c = (mDNSu8)*cstr++;							// just use the second character
				else if (mdnsIsDigit(cstr[0]) && mdnsIsDigit(cstr[1]) && mdnsIsDigit(cstr[2]))
				{												// else, if three decimal digits,
					int v0 = cstr[0] - '0';							// then interpret as three-digit decimal
					int v1 = cstr[1] - '0';
					int v2 = cstr[2] - '0';
					int val = v0 * 100 + v1 * 10 + v2;
					if (val <= 255) { c = (mDNSu8)val; cstr += 3; }	// If valid value, use it
				}
			}
			*ptr++ = c;												// Write the character
		}
		if (*cstr) cstr++;											// Skip over the trailing dot (if present)
		if (ptr - lengthbyte - 1 > MAX_DOMAIN_LABEL)				// If illegal label, abort
			return(mDNSNULL);
		*lengthbyte = (mDNSu8)(ptr - lengthbyte - 1);				// Fill in the length byte
	}

	*ptr++ = 0;														// Put the null root label on the end
	if (*cstr) return(mDNSNULL);									// Failure: We didn't successfully consume all input
	else return(ptr);												// Success: return new value of ptr
}

// AppendDomainLabel appends a single label to a name.
// If successful, AppendDomainLabel returns a pointer to the next unused byte
// in the domainname bufer (i.e., the next byte after the terminating zero).
// If unable to construct a legal domain name (i.e. label more than 63 bytes, or total more than 255 bytes)
// AppendDomainLabel returns mDNSNULL.
mDNSexport mDNSu8 *AppendDomainLabel(domainname *const name, const domainlabel *const label)
{
	int i;
	mDNSu8 *ptr = name->c + DomainNameLength(name) - 1;

	// Check label is legal
	if (label->c[0] > MAX_DOMAIN_LABEL) return(mDNSNULL);

	// Check that ptr + length byte + data bytes + final zero does not exceed our limit
	if (ptr + 1 + label->c[0] + 1 > name->c + MAX_DOMAIN_NAME) return(mDNSNULL);

	for (i=0; i<=label->c[0]; i++) *ptr++ = label->c[i];	// Copy the label data
	*ptr++ = 0;								// Put the null root label on the end
	return(ptr);
}

mDNSexport mDNSu8 *AppendDomainName(domainname *const name, const domainname *const append)
{
	mDNSu8       *      ptr = name->c + DomainNameLength(name) - 1;	// Find end of current name
	const mDNSu8 *const lim = name->c + MAX_DOMAIN_NAME - 1;		// Limit of how much we can add (not counting final zero)
	const mDNSu8 *      src = append->c;
	while(src[0])
	{
		int i;
		if (ptr + 1 + src[0] > lim) return(mDNSNULL);
		for (i=0; i<=src[0]; i++) *ptr++ = src[i];
		*ptr = 0;	// Put the null root label on the end
		src += i;
	}
	return(ptr);
}

// MakeDomainLabelFromLiteralString makes a single domain label from a single literal C string (with no escaping).
// If successful, MakeDomainLabelFromLiteralString returns mDNStrue.
// If unable to convert the whole string to a legal domain label (i.e. because length is more than 63 bytes) then
// MakeDomainLabelFromLiteralString makes a legal domain label from the first 63 bytes of the string and returns mDNSfalse.
// In some cases silently truncated oversized names to 63 bytes is acceptable, so the return result may be ignored.
// In other cases silent truncation may not be acceptable, so in those cases the calling function needs to check the return result.
mDNSexport mDNSBool MakeDomainLabelFromLiteralString(domainlabel *const label, const char *cstr)
{
	mDNSu8       *      ptr   = label->c + 1;						// Where we're putting it
	const mDNSu8 *const limit = label->c + 1 + MAX_DOMAIN_LABEL;	// The maximum we can put
	while (*cstr && ptr < limit) *ptr++ = (mDNSu8)*cstr++;			// Copy the label
	label->c[0] = (mDNSu8)(ptr - label->c - 1);						// Set the length byte
	return(*cstr == 0);												// Return mDNStrue if we successfully consumed all input
}

// MakeDomainNameFromDNSNameString makes a native DNS-format domainname from a C string.
// The C string is in conventional DNS syntax:
// Textual labels, escaped as necessary using the usual DNS '\' notation, separated by dots.
// If successful, MakeDomainNameFromDNSNameString returns a pointer to the next unused byte
// in the domainname bufer (i.e., the next byte after the terminating zero).
// If unable to construct a legal domain name (i.e. label more than 63 bytes, or total more than 255 bytes)
// MakeDomainNameFromDNSNameString returns mDNSNULL.
mDNSexport mDNSu8 *MakeDomainNameFromDNSNameString(domainname *const name, const char *cstr)
{
	name->c[0] = 0;									// Make an empty domain name
	return(AppendDNSNameString(name, cstr));		// And then add this string to it
}
#if 0
mDNSexport char *ConvertDomainLabelToCString_withescape(const domainlabel *const label, char *ptr, char esc)
{
	const mDNSu8 *      src = label->c;							// Domain label we're reading
	const mDNSu8        len = *src++;							// Read length of this (non-null) label
	const mDNSu8 *const end = src + len;						// Work out where the label ends
	if (len > MAX_DOMAIN_LABEL) return(mDNSNULL);				// If illegal label, abort
	while (src < end)											// While we have characters in the label
	{
		mDNSu8 c = *src++;
		if (esc)
		{
			if (c == '.' || c == esc)							// If character is a dot or the escape character
				*ptr++ = esc;									// Output escape character
			else if (c <= ' ')									// If non-printing ascii,
			{												// Output decimal escape sequence
				*ptr++ = esc;
				*ptr++ = (char)  ('0' + (c / 100)     );
				*ptr++ = (char)  ('0' + (c /  10) % 10);
				c      = (mDNSu8)('0' + (c      ) % 10);
			}
		}
		*ptr++ = (char)c;										// Copy the character
	}
	*ptr = 0;													// Null-terminate the string
	return(ptr);												// and return
}

// Note, to guarantee that there will be no possible overrun, cstr must be at least 1005 bytes
// The longest legal domain name is 255 bytes, in the form of three 64-byte labels, one 62-byte label,
// and the null root label.
// If every label character has to be escaped as a four-byte escape sequence, the maximum textual
// ascii display of this is 63*4 + 63*4 + 63*4 + 61*4 = 1000 label characters,
// plus four dots and the null at the end of the C string = 1005
mDNSexport char *ConvertDomainNameToCString_withescape(const domainname *const name, char *ptr, char esc)
{
	const mDNSu8 *src         = name->c;								// Domain name we're reading
	const mDNSu8 *const max   = name->c + MAX_DOMAIN_NAME;			// Maximum that's valid
	
	if (*src == 0) *ptr++ = '.';									// Special case: For root, just write a dot

	while (*src)													// While more characters in the domain name
	{
		if (src + 1 + *src >= max) return(mDNSNULL);
		ptr = ConvertDomainLabelToCString_withescape((const domainlabel *)src, ptr, esc);
		if (!ptr) return(mDNSNULL);
		src += 1 + *src;
		*ptr++ = '.';												// Write the dot after the label
	}

	*ptr++ = 0;														// Null-terminate the string
	return(ptr);													// and return
}

// RFC 1034 rules:
// Host names must start with a letter, end with a letter or digit,
// and have as interior characters only letters, digits, and hyphen.
// This was subsequently modified in RFC 1123 to allow the first character to be either a letter or a digit
#define mdnsValidHostChar(X, notfirst, notlast) (mdnsIsLetter(X) || mdnsIsDigit(X) || ((notfirst) && (notlast) && (X) == '-') )

mDNSexport void ConvertUTF8PstringToRFC1034HostLabel(const mDNSu8 UTF8Name[], domainlabel *const hostlabel)
{
	const mDNSu8 *      src  = &UTF8Name[1];
	const mDNSu8 *const end  = &UTF8Name[1] + UTF8Name[0];
	mDNSu8 *      ptr  = &hostlabel->c[1];
	const mDNSu8 *const lim  = &hostlabel->c[1] + MAX_DOMAIN_LABEL;
	while (src < end)
	{
		// Delete apostrophes from source name
		if (src[0] == '\'') { src++; continue; }		// Standard straight single quote
		if (src + 2 < end && src[0] == 0xE2 && src[1] == 0x80 && src[2] == 0x99)
		{ src += 3; continue; }	// Unicode curly apostrophe
		if (ptr < lim)
		{
			if (mdnsValidHostChar(*src, (ptr > &hostlabel->c[1]), (src < end-1))) *ptr++ = *src;
			else if (ptr > &hostlabel->c[1] && ptr[-1] != '-') *ptr++ = '-';
		}
		src++;
	}
	while (ptr > &hostlabel->c[1] && ptr[-1] == '-') ptr--;	// Truncate trailing '-' marks
	hostlabel->c[0] = (mDNSu8)(ptr - &hostlabel->c[1]);
}
#endif
mDNSexport mDNSu8 *ConstructServiceName(domainname *const fqdn,
					const domainlabel *name, const domainname *type, const domainname *const domain)
{
	int i, len;
	mDNSu8 *dst = fqdn->c;
	const mDNSu8 *src;
#if MDNS_DEBUGMSGS > 2
	const char *errormsg;
#endif
	// In the case where there is no name (and ONLY in that case),
	// a single-label subtype is allowed as the first label of a three-part "type"
	if (!name)
	{
		const mDNSu8 *s2 = type->c + 1 + type->c[0];
		if (type->c[0]  > 0 && type->c[0]  < 0x40 &&
		    s2[0]       > 0 && s2[0]       < 0x40 &&
		    s2[1+s2[0]] > 0 && s2[1+s2[0]] < 0x40)
		{
			name = (domainlabel *)type;
			type = (domainname  *)s2;
		}
	}

	if (name && name->c[0])
	{
		src = name->c;									// Put the service name into the domain name
		len = *src;
		if (len >= 0x40) 
		{ 
#if MDNS_DEBUGMSGS > 2
			errormsg="Service instance name too long"; 
#endif
			goto fail;
		}
		for (i=0; i<=len; i++) *dst++ = *src++;
	}
	else
		name = (domainlabel*)"";	// Set this up to be non-null, to avoid errors if we have to call LogMsg() below
	
	src = type->c;										// Put the service type into the domain name
	len = *src;
	if (len < 2 || len >= 0x40)  
	{
#if MDNS_DEBUGMSGS > 2
		errormsg="Invalid service application protocol name";
#endif
		goto fail; 
	}
	if (src[1] != '_') 
	{ 
#if MDNS_DEBUGMSGS > 2
		errormsg="Service application protocol name must begin with underscore";
#endif
		goto fail; 
	}
	for (i=2; i<=len; i++)
		if (!mdnsIsLetter(src[i]) && !mdnsIsDigit(src[i]) && src[i] != '-' && src[i] != '_')
		{ 
#if MDNS_DEBUGMSGS > 2
			errormsg="Service application protocol name must contain only letters, digits, and hyphens"; 
#endif
			goto fail; 
		}
	for (i=0; i<=len; i++) *dst++ = *src++;

	len = *src;
	//if (len == 0 || len >= 0x40)  { errormsg="Invalid service transport protocol name"; goto fail; }
	if (!(len == 4 && src[1] == '_' &&
	      (((src[2] | 0x20) == 'u' && (src[3] | 0x20) == 'd') || ((src[2] | 0x20) == 't' && (src[3] | 0x20) == 'c')) &&
	      (src[4] | 0x20) == 'p'))
	{ 
#if MDNS_DEBUGMSGS > 2
		errormsg="Service transport protocol name must be _udp or _tcp"; 
#endif
		goto fail; 
	}
	for (i=0; i<=len; i++) *dst++ = *src++;
	
	if (*src) 
	{
#if MDNS_DEBUGMSGS > 2 
		errormsg="Service type must have only two labels"; 
#endif
		goto fail; 
	}
	
	*dst = 0;
	dst = AppendDomainName(fqdn, domain);
	if (!dst) 
	{
#if MDNS_DEBUGMSGS > 2
		errormsg="Service domain too long"; 
#endif
		goto fail; 
	}
	return(dst);

 fail:
	LogMsg("ConstructServiceName: %s: %#s.%##s%##s", errormsg, name->c, type->c, domain->c);
	return(mDNSNULL);
}

mDNSexport mDNSBool DeconstructServiceName(const domainname *const fqdn,
					   domainlabel *const name, domainname *const type, domainname *const domain)
{
	int i, len;
	const mDNSu8 *src = fqdn->c;
	const mDNSu8 *max = fqdn->c + MAX_DOMAIN_NAME;
	mDNSu8 *dst;
	
	dst = name->c;										// Extract the service name from the domain name
	len = *src;
	if (len >= 0x40) { debugf("DeconstructServiceName: service name too long"); return(mDNSfalse); }
	for (i=0; i<=len; i++) *dst++ = *src++;
	
	dst = type->c;										// Extract the service type from the domain name
	len = *src;
	if (len >= 0x40) { debugf("DeconstructServiceName: service type too long"); return(mDNSfalse); }
	for (i=0; i<=len; i++) *dst++ = *src++;

	len = *src;
	if (len >= 0x40) { debugf("DeconstructServiceName: service type too long"); return(mDNSfalse); }
	for (i=0; i<=len; i++) *dst++ = *src++;
	*dst++ = 0;		// Put the null root label on the end of the service type

	dst = domain->c;									// Extract the service domain from the domain name
	while (*src)
	{
		len = *src;
		if (len >= 0x40)
		{ debugf("DeconstructServiceName: service domain label too long"); return(mDNSfalse); }
		if (src + 1 + len + 1 >= max)
		{ debugf("DeconstructServiceName: service domain too long"); return(mDNSfalse); }
		for (i=0; i<=len; i++) *dst++ = *src++;
	}
	*dst++ = 0;		// Put the null root label on the end
	
	return(mDNStrue);
}

// Returns true if a rich text label ends in " (nnn)", or if an RFC 1034
// name ends in "-nnn", where n is some decimal number.
mDNSlocal mDNSBool LabelContainsSuffix(const domainlabel *const name, const mDNSBool RichText)
{
	mDNSu16 l = name->c[0];
	
	if (RichText)
	{
		if (l < 4) return mDNSfalse;							// Need at least " (2)"
		if (name->c[l--] != ')') return mDNSfalse;				// Last char must be ')'
		if (!mdnsIsDigit(name->c[l])) return mDNSfalse;			// Preceeded by a digit
		l--;
		while (l > 2 && mdnsIsDigit(name->c[l])) l--;			// Strip off digits
		return (name->c[l] == '(' && name->c[l - 1] == ' ');
	}
	else
	{
		if (l < 2) return mDNSfalse;							// Need at least "-2"
		if (!mdnsIsDigit(name->c[l])) return mDNSfalse;			// Last char must be a digit
		l--;
		while (l > 2 && mdnsIsDigit(name->c[l])) l--;			// Strip off digits
		return (name->c[l] == '-');
	}
}

// removes an auto-generated suffix (appended on a name collision) from a label.  caller is
// responsible for ensuring that the label does indeed contain a suffix.  returns the number
// from the suffix that was removed.
mDNSlocal mDNSu32 RemoveLabelSuffix(domainlabel *name, mDNSBool RichText)
{
	mDNSu32 val = 0, multiplier = 1;

	// Chop closing parentheses from RichText suffix
	if (RichText && name->c[0] >= 1 && name->c[name->c[0]] == ')') name->c[0]--;

	// Get any existing numerical suffix off the name
	while (mdnsIsDigit(name->c[name->c[0]]))
	{ val += (name->c[name->c[0]] - '0') * multiplier; multiplier *= 10; name->c[0]--; }

	// Chop opening parentheses or dash from suffix
	if (RichText)
	{
		if (name->c[0] >= 2 && name->c[name->c[0]] == '(' && name->c[name->c[0]-1] == ' ') name->c[0] -= 2;
	}
	else
	{
		if (name->c[0] >= 1 && name->c[name->c[0]] == '-') name->c[0] -= 1;
	}

	return(val);
}

// appends a numerical suffix to a label, with the number following a whitespace and enclosed
// in parentheses (rich text) or following two consecutive hyphens (RFC 1034 domain label).
mDNSlocal void AppendLabelSuffix(domainlabel *name, mDNSu32 val, mDNSBool RichText)
{
	mDNSu32 divisor = 1, chars = 2;	// Shortest possible RFC1034 name suffix is 3 characters ("-2")
	if (RichText) chars = 4;		// Shortest possible RichText suffix is 4 characters (" (2)")
	
	// Truncate trailing spaces from RichText names
	if (RichText) while (name->c[name->c[0]] == ' ') name->c[0]--;

	while (val >= divisor * 10) { divisor *= 10; chars++; }

	if (name->c[0] > (mDNSu8)(MAX_DOMAIN_LABEL - chars))
	{
		name->c[0] = (mDNSu8)(MAX_DOMAIN_LABEL - chars);
		// If the following character is a UTF-8 continuation character,
		// we just chopped a multi-byte UTF-8 character in the middle, so strip back to a safe truncation point
		while (name->c[0] > 0 && (name->c[name->c[0]+1] & 0xC0) == 0x80) name->c[0]--;
	}

	if (RichText) { name->c[++name->c[0]] = ' '; name->c[++name->c[0]] = '('; }
	else          { name->c[++name->c[0]] = '-'; }
	
	while (divisor)
	{
		name->c[++name->c[0]] = (mDNSu8)('0' + val / divisor);
		val     %= divisor;
		divisor /= 10;
	}

	if (RichText) name->c[++name->c[0]] = ')';
}

mDNSexport void IncrementLabelSuffix(domainlabel *name, mDNSBool RichText)
{
	mDNSu32 val = 0;

	if (LabelContainsSuffix(name, RichText))
		val = RemoveLabelSuffix(name, RichText);
		
	// If no existing suffix, start by renaming "Foo" as "Foo (2)" or "Foo-2" as appropriate.
	// If existing suffix in the range 2-9, increment it.
	// If we've had ten conflicts already, there are probably too many hosts trying to use the same name,
	// so add a random increment to improve the chances of finding an available name next time.
	if      (val == 0) val = 2;
	else if (val < 10) val++;
	else               val += 1 + mDNSRandom(99);
	
	AppendLabelSuffix(name, val, RichText);
}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Resource Record Utility Functions
#endif

#define RRIsAddressType(RR) ((RR)->resrec.rrtype == kDNSType_A || (RR)->resrec.rrtype == kDNSType_AAAA)
#define RRTypeIsAddressType(T) ((T) == kDNSType_A || (T) == kDNSType_AAAA)

#define ResourceRecordIsValidAnswer(RR) ( ((RR)->             resrec.RecordType & kDNSRecordTypeActiveMask)  && \
		((RR)->Additional1 == mDNSNULL || ((RR)->Additional1->resrec.RecordType & kDNSRecordTypeActiveMask)) && \
		((RR)->Additional2 == mDNSNULL || ((RR)->Additional2->resrec.RecordType & kDNSRecordTypeActiveMask)) && \
		((RR)->DependentOn == mDNSNULL || ((RR)->DependentOn->resrec.RecordType & kDNSRecordTypeActiveMask))  )

#define ResourceRecordIsValidInterfaceAnswer(RR, INTID) \
	(ResourceRecordIsValidAnswer(RR) && \
	((RR)->resrec.InterfaceID == mDNSInterface_Any || (RR)->resrec.InterfaceID == (INTID)))

#define RRUniqueOrKnownUnique(RR) ((RR)->RecordType & (kDNSRecordTypeUnique | kDNSRecordTypeKnownUnique))

#define DefaultProbeCountForTypeUnique ((mDNSu8)3)
#define DefaultProbeCountForRecordType(X)      ((X) == kDNSRecordTypeUnique ? DefaultProbeCountForTypeUnique : (mDNSu8)0)

// For records that have *never* been announced on the wire, their AnnounceCount will be set to InitialAnnounceCount (10).
// When de-registering these records we do not need to send any goodbye packet because we never announced them in the first
// place. If AnnounceCount is less than InitialAnnounceCount that means we have announced them at least once, so a goodbye
// packet is needed. For this reason, if we ever reset AnnounceCount (e.g. after an interface change) we set it to
// ReannounceCount (9), not InitialAnnounceCount. If we were to reset AnnounceCount back to InitialAnnounceCount that would
// imply that the record had never been announced on the wire (which is false) and if the client were then to immediately
// deregister that record before it had a chance to announce, we'd fail to send its goodbye packet (which would be a bug).
#define InitialAnnounceCount ((mDNSu8)10)
#define ReannounceCount      ((mDNSu8)9)

// Note that the announce intervals use exponential backoff, doubling each time. The probe intervals do not.
// This means that because the announce interval is doubled after sending the first packet, the first
// observed on-the-wire inter-packet interval between announcements is actually one second.
// The half-second value here may be thought of as a conceptual (non-existent) half-second delay *before* the first packet is sent.
#define DefaultProbeIntervalForTypeUnique (mDNSPlatformOneSecond/4)
#define DefaultAnnounceIntervalForTypeShared (mDNSPlatformOneSecond/2)
#define DefaultAnnounceIntervalForTypeUnique (mDNSPlatformOneSecond/2)

#define DefaultAPIntervalForRecordType(X)  ((X) & (kDNSRecordTypeAdvisory | kDNSRecordTypeShared     ) ? DefaultAnnounceIntervalForTypeShared : \
											(X) & (kDNSRecordTypeUnique                              ) ? DefaultProbeIntervalForTypeUnique    : \
											(X) & (kDNSRecordTypeVerified | kDNSRecordTypeKnownUnique) ? DefaultAnnounceIntervalForTypeUnique : 0)

#define TimeToAnnounceThisRecord(RR,time) ((RR)->AnnounceCount && (time) - ((RR)->LastAPTime + (RR)->ThisAPInterval) >= 0)
#define TimeToSendThisRecord(RR,time) ((TimeToAnnounceThisRecord(RR,time) || (RR)->ImmedAnswer) && ResourceRecordIsValidAnswer(RR))
#define TicksTTL(RR) ((mDNSs32)(RR)->resrec.rroriginalttl * mDNSPlatformOneSecond)
#define RRExpireTime(RR) ((RR)->TimeRcvd + TicksTTL(RR))

#define MaxUnansweredQueries 4

mDNSlocal mDNSBool SameRData(const ResourceRecord *const r1, const ResourceRecord *const r2)
{
	if (r1->rrtype     != r2->rrtype)   return(mDNSfalse);
	if (r1->rdlength   != r2->rdlength) return(mDNSfalse);
	if (r1->rdatahash  != r2->rdatahash) return(mDNSfalse);
	if (r1->rdnamehash != r2->rdnamehash) return(mDNSfalse);
	switch(r1->rrtype)
	{
	case kDNSType_CNAME:// Same as PTR
	case kDNSType_PTR:	return(SameDomainName(&r1->rdata->u.name, &r2->rdata->u.name));

	case kDNSType_SRV:	return(mDNSBool)(  	r1->rdata->u.srv.priority          == r2->rdata->u.srv.priority          &&
							r1->rdata->u.srv.weight            == r2->rdata->u.srv.weight            &&
							r1->rdata->u.srv.port.NotAnInteger == r2->rdata->u.srv.port.NotAnInteger &&
							SameDomainName(&r1->rdata->u.srv.target, &r2->rdata->u.srv.target)       );

	default:			return(mDNSPlatformMemSame(r1->rdata->u.data, r2->rdata->u.data, r1->rdlength));
	}
}

mDNSlocal mDNSBool ResourceRecordAnswersQuestion(const ResourceRecord *const rr, const DNSQuestion *const q)
{
	if (rr->InterfaceID &&
	    q ->InterfaceID &&
	    rr->InterfaceID != q->InterfaceID) return(mDNSfalse);

	// RR type CNAME matches any query type. QTYPE ANY matches any RR type. QCLASS ANY matches any RR class.
	if (rr->rrtype != kDNSType_CNAME && rr->rrtype  != q->qtype  && q->qtype  != kDNSQType_ANY ) return(mDNSfalse);
	if (                                rr->rrclass != q->qclass && q->qclass != kDNSQClass_ANY) return(mDNSfalse);
	return(rr->namehash == q->qnamehash && SameDomainName(&rr->name, &q->qname));
}

mDNSlocal mDNSu32 DomainNameHashValue(const domainname *const name)
{
	mDNSu32 sum = 0;
	const mDNSu8 *c;

	for (c = name->c; c[0] != 0 && c[1] != 0; c += 2)
	{
		sum += ((mDNSIsUpperCase(c[0]) ? c[0] + 'a' - 'A' : c[0]) << 8) |
			(mDNSIsUpperCase(c[1]) ? c[1] + 'a' - 'A' : c[1]);
		sum = (sum<<3) | (sum>>29);
	}
	if (c[0]) sum += ((mDNSIsUpperCase(c[0]) ? c[0] + 'a' - 'A' : c[0]) << 8);
	return(sum);
}

#define HashSlot(X) (DomainNameHashValue(X) % CACHE_HASH_SLOTS)

mDNSlocal mDNSu32 RDataHashValue(mDNSu16 const rdlength, const RDataBody *const rdb)
{
	mDNSu32 sum = 0;
	int i;
	for (i=0; i+1 < rdlength; i+=2)
	{
		sum += (((mDNSu32)(rdb->data[i])) << 8) | rdb->data[i+1];
		sum = (sum<<3) | (sum>>29);
	}
	if (i < rdlength)
	{
		sum += ((mDNSu32)(rdb->data[i])) << 8;
	}
	return(sum);
}

// SameResourceRecordSignature returns true if two resources records have the same name, type, and class, and may be sent
// (or were received) on the same interface (i.e. if *both* records specify an interface, then it has to match).
// TTL and rdata may differ.
// This is used for cache flush management:
// When sending a unique record, all other records matching "SameResourceRecordSignature" must also be sent
// When receiving a unique record, all old cache records matching "SameResourceRecordSignature" are flushed
mDNSlocal mDNSBool SameResourceRecordSignature(const ResourceRecord *const r1, const ResourceRecord *const r2)
{
	if (!r1) { LogMsg("SameResourceRecordSignature ERROR: r1 is NULL"); return(mDNSfalse); }
	if (!r2) { LogMsg("SameResourceRecordSignature ERROR: r2 is NULL"); return(mDNSfalse); }
	if (r1->InterfaceID &&
	    r2->InterfaceID &&
	    r1->InterfaceID != r2->InterfaceID) return(mDNSfalse);
	return(mDNSBool)(r1->rrtype == r2->rrtype && r1->rrclass == r2->rrclass && r1->namehash == r2->namehash && SameDomainName(&r1->name, &r2->name));
}

// PacketRRMatchesSignature behaves as SameResourceRecordSignature, except that types may differ if the
// authoratative record is in the probing state.  Probes are sent with the wildcard type, so a response of
// any type should match, even if it is not the type the client plans to use.
mDNSlocal mDNSBool PacketRRMatchesSignature(const CacheRecord *const pktrr, const AuthRecord *const authrr)
{
	if (!pktrr)  { LogMsg("PacketRRMatchesSignature ERROR: pktrr is NULL"); return(mDNSfalse); }
	if (!authrr) { LogMsg("PacketRRMatchesSignature ERROR: authrr is NULL"); return(mDNSfalse); }
	if (pktrr->resrec.InterfaceID &&
	    authrr->resrec.InterfaceID &&
	    pktrr->resrec.InterfaceID != authrr->resrec.InterfaceID) return(mDNSfalse);
	if (authrr->resrec.RecordType != kDNSRecordTypeUnique && pktrr->resrec.rrtype != authrr->resrec.rrtype) return(mDNSfalse);
	return(mDNSBool)(pktrr->resrec.rrclass == authrr->resrec.rrclass && pktrr->resrec.namehash == authrr->resrec.namehash && SameDomainName(&pktrr->resrec.name, &authrr->resrec.name));
}

// IdenticalResourceRecord returns true if two resources records have
// the same name, type, class, and identical rdata (InterfaceID and TTL may differ)
mDNSlocal mDNSBool IdenticalResourceRecord(const ResourceRecord *const r1, const ResourceRecord *const r2)
{
	if (!r1) { LogMsg("IdenticalResourceRecord ERROR: r1 is NULL"); return(mDNSfalse); }
	if (!r2) { LogMsg("IdenticalResourceRecord ERROR: r2 is NULL"); return(mDNSfalse); }
	if (r1->rrtype != r2->rrtype || r1->rrclass != r2->rrclass || r1->namehash != r2->namehash || !SameDomainName(&r1->name, &r2->name)) return(mDNSfalse);
	return(SameRData(r1, r2));
}

// CacheRecord *ks is the CacheRecord from the known answer list in the query.
// This is the information that the requester believes to be correct.
// AuthRecord *rr is the answer we are proposing to give, if not suppressed.
// This is the information that we believe to be correct.
// We've already determined that we plan to give this answer on this interface
// (either the record is non-specific, or it is specific to this interface)
// so now we just need to check the name, type, class, rdata and TTL.
mDNSlocal mDNSBool ShouldSuppressKnownAnswer(const CacheRecord *const ka, const AuthRecord *const rr)
{
	// If RR signature is different, or data is different, then don't suppress our answer
	if (!IdenticalResourceRecord(&ka->resrec,&rr->resrec)) return(mDNSfalse);
	
	// If the requester's indicated TTL is less than half the real TTL,
	// we need to give our answer before the requester's copy expires.
	// If the requester's indicated TTL is at least half the real TTL,
	// then we can suppress our answer this time.
	// If the requester's indicated TTL is greater than the TTL we believe,
	// then that's okay, and we don't need to do anything about it.
	// (If two responders on the network are offering the same information,
	// that's okay, and if they are offering the information with different TTLs,
	// the one offering the lower TTL should defer to the one offering the higher TTL.)
	return(mDNSBool)(ka->resrec.rroriginalttl >= rr->resrec.rroriginalttl / 2);
}

mDNSlocal mDNSu16 GetRDLength(const ResourceRecord *const rr, mDNSBool estimate)
{
	RDataBody *rd = &rr->rdata->u;
	const domainname *const name = estimate ? &rr->name : mDNSNULL;
	switch (rr->rrtype)
	{
	case kDNSType_A:	return(sizeof(rd->ip));
	case kDNSType_CNAME:// Same as PTR
	case kDNSType_PTR:	return(CompressedDomainNameLength(&rd->name, name));
	case kDNSType_HINFO:return(mDNSu16)(2 + (int)rd->data[0] + (int)rd->data[1 + (int)rd->data[0]]);
	case kDNSType_NULL:	// Same as TXT -- not self-describing, so have to just trust rdlength
	case kDNSType_TXT:  return(rr->rdlength); // TXT is not self-describing, so have to just trust rdlength
	case kDNSType_AAAA:	return(sizeof(rd->ipv6));
	case kDNSType_SRV:	return(mDNSu16)(6 + CompressedDomainNameLength(&rd->srv.target, name));
	default:			debugf("Warning! Don't know how to get length of resource type %d", rr->rrtype);
		return(rr->rdlength);
	}
}

mDNSlocal void SetNextAnnounceProbeTime(mDNS *const m, const AuthRecord *const rr)
{
	if (rr->resrec.RecordType == kDNSRecordTypeUnique)
	{
		if (m->NextScheduledProbe - (rr->LastAPTime + rr->ThisAPInterval) >= 0)
			m->NextScheduledProbe = (rr->LastAPTime + rr->ThisAPInterval);
	}
	else if (rr->AnnounceCount && ResourceRecordIsValidAnswer(rr))
	{
		if (m->NextScheduledResponse - (rr->LastAPTime + rr->ThisAPInterval) >= 0)
			m->NextScheduledResponse = (rr->LastAPTime + rr->ThisAPInterval);
	}
}

#define GetRRDomainNameTarget(RR) (                                                                          \
	((RR)->rrtype == kDNSType_CNAME || (RR)->rrtype == kDNSType_PTR) ? &(RR)->rdata->u.name       :          \
	((RR)->rrtype == kDNSType_SRV                                  ) ? &(RR)->rdata->u.srv.target : mDNSNULL )

mDNSlocal void InitializeLastAPTime(mDNS *const m, AuthRecord *const rr)
{
	// To allow us to aggregate probes when a group of services are registered together,
	// the first probe is delayed 1/4 second. This means the common-case behaviour is:
	// 1/4 second wait; probe
	// 1/4 second wait; probe
	// 1/4 second wait; probe
	// 1/4 second wait; announce (i.e. service is normally announced exactly one second after being registered)

	// If we have no probe suppression time set, or it is in the past, set it now
	if (m->SuppressProbes == 0 || m->SuppressProbes - m->timenow < 0)
	{
		m->SuppressProbes = (m->timenow + DefaultProbeIntervalForTypeUnique) | 1;
		// If we already have a probe scheduled to go out sooner, then use that time to get better aggregation
		if (m->SuppressProbes - m->NextScheduledProbe >= 0)
			m->SuppressProbes = m->NextScheduledProbe;
		// If we already have a query scheduled to go out sooner, then use that time to get better aggregation
		if (m->SuppressProbes - m->NextScheduledQuery >= 0)
			m->SuppressProbes = m->NextScheduledQuery;
	}
	
	// We announce to flush stale data from other caches. It is a reasonable assumption that any
	// old stale copies will probably have the same TTL we're using, so announcing longer than
	// this serves no purpose -- any stale copies of that record will have expired by then anyway.
	rr->AnnounceUntil   = m->timenow + TicksTTL(rr);
	rr->LastAPTime      = m->SuppressProbes - rr->ThisAPInterval;
	// Set LastMCTime to now, to inhibit multicast responses
	// (no need to send additional multicast responses when we're announcing anyway)
	rr->LastMCTime      = m->timenow;
	rr->LastMCInterface = mDNSInterfaceMark;
	
	// If this is a record type that's not going to probe, then delay its first announcement so that
	// it will go out synchronized with the first announcement for the other records that *are* probing.
	// This is a minor performance tweak that helps keep groups of related records synchronized together.
	// The addition of "rr->ThisAPInterval / 2" is to make sure that, in the event that any of the probes are
	// delayed by a few milliseconds, this announcement does not inadvertently go out *before* the probing is complete.
	// When the probing is complete and those records begin to announce, these records will also be picked up and accelerated,
	// because they will meet the criterion of being at least half-way to their scheduled announcement time.
	if (rr->resrec.RecordType != kDNSRecordTypeUnique)
		rr->LastAPTime += DefaultProbeIntervalForTypeUnique * DefaultProbeCountForTypeUnique + rr->ThisAPInterval / 2;
	
	SetNextAnnounceProbeTime(m, rr);
}

mDNSlocal void SetNewRData(ResourceRecord *const rr, RData *NewRData, mDNSu16 rdlength)
{
	domainname *target;
	if (NewRData)
	{
		rr->rdata    = NewRData;
		rr->rdlength = rdlength;
	}
	// Must not try to get target pointer until after updating rr->rdata
	target = GetRRDomainNameTarget(rr);
	rr->rdlength   = GetRDLength(rr, mDNSfalse);
	rr->rdestimate = GetRDLength(rr, mDNStrue);
	rr->rdatahash  = RDataHashValue(rr->rdlength, &rr->rdata->u);
	rr->rdnamehash = target ? DomainNameHashValue(target) : 0;
}

mDNSlocal void SetTargetToHostName(mDNS *const m, AuthRecord *const rr)
{
	domainname *target = GetRRDomainNameTarget(&rr->resrec);

	if (!target) debugf("SetTargetToHostName: Don't know how to set the target of rrtype %d", rr->resrec.rrtype);

	if (target && SameDomainName(target, &m->hostname))
		debugf("SetTargetToHostName: Target of %##s is already %##s", rr->resrec.name.c, target->c);
	
	if (target && !SameDomainName(target, &m->hostname))
	{
		AssignDomainName(*target, m->hostname);
		SetNewRData(&rr->resrec, mDNSNULL, 0);
		
		// If we're in the middle of probing this record, we need to start again,
		// because changing its rdata may change the outcome of the tie-breaker.
		// (If the record type is kDNSRecordTypeUnique (unconfirmed unique) then DefaultProbeCountForRecordType is non-zero.)
		rr->ProbeCount     = DefaultProbeCountForRecordType(rr->resrec.RecordType);

		// If we've announced this record, we really should send a goodbye packet for the old rdata before
		// changing to the new rdata. However, in practice, we only do SetTargetToHostName for unique records,
		// so when we announce them we'll set the kDNSClass_UniqueRRSet and clear any stale data that way.
		if (rr->AnnounceCount < InitialAnnounceCount && rr->resrec.RecordType == kDNSRecordTypeShared)
			debugf("Have announced shared record %##s (%s) at least once: should have sent a goodbye packet before updating", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));

		if (rr->AnnounceCount < ReannounceCount)
			rr->AnnounceCount = ReannounceCount;
		rr->ThisAPInterval = DefaultAPIntervalForRecordType(rr->resrec.RecordType);
		InitializeLastAPTime(m,rr);
	}
}

mDNSlocal void CompleteProbing(mDNS *const m, AuthRecord *const rr)
{
	verbosedebugf("Probing for %##s (%s) complete", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
	if (!rr->Acknowledged && rr->RecordCallback)
	{
		// CAUTION: MUST NOT do anything more with rr after calling rr->Callback(), because the client's callback function
		// is allowed to do anything, including starting/stopping queries, registering/deregistering records, etc.
		rr->Acknowledged = mDNStrue;
		m->mDNS_reentrancy++; // Increment to allow client to legally make mDNS API calls from the callback
		rr->RecordCallback(m, rr, mStatus_NoError);
		m->mDNS_reentrancy--; // Decrement to block mDNS API calls again
	}
}

#define ValidateDomainName(N) (DomainNameLength(N) <= MAX_DOMAIN_NAME)

mDNSlocal mDNSBool ValidateRData(const mDNSu16 rrtype, const mDNSu16 rdlength, const RData *const rd)
{
	mDNSu16 len;
	switch(rrtype)
	{
	case kDNSType_A:	return(rdlength == sizeof(mDNSv4Addr));

	case kDNSType_NS:	// Same as PTR
	case kDNSType_MD:	// Same as PTR
	case kDNSType_MF:	// Same as PTR
	case kDNSType_CNAME:// Same as PTR
		//case kDNSType_SOA not checked
	case kDNSType_MB:	// Same as PTR
	case kDNSType_MG:	// Same as PTR
	case kDNSType_MR:	// Same as PTR
		//case kDNSType_NULL not checked (no specified format, so always valid)
		//case kDNSType_WKS not checked
	case kDNSType_PTR:	len = DomainNameLength(&rd->u.name);
		return(len <= MAX_DOMAIN_NAME && rdlength == len);

	case kDNSType_HINFO:// Same as TXT (roughly)
	case kDNSType_MINFO:// Same as TXT (roughly)
	case kDNSType_TXT:  {
		const mDNSu8 *ptr = rd->u.txt.c;
		const mDNSu8 *end = rd->u.txt.c + rdlength;
		while (ptr < end) ptr += 1 + ptr[0];
		return (ptr == end);
	}

	case kDNSType_AAAA:	return(rdlength == sizeof(mDNSv6Addr));

	case kDNSType_MX:   len = DomainNameLength(&rd->u.mx.exchange);
		return(len <= MAX_DOMAIN_NAME && rdlength == 2+len);

	case kDNSType_SRV:	len = DomainNameLength(&rd->u.srv.target);
		return(len <= MAX_DOMAIN_NAME && rdlength == 6+len);

	default:			return(mDNStrue);	// Allow all other types without checking
	}
}

// Two records qualify to be local duplicates if the RecordTypes are the same, or if one is Unique and the other Verified
#define RecordLDT(A,B) ((A)->resrec.RecordType == (B)->resrec.RecordType || ((A)->resrec.RecordType | (B)->resrec.RecordType) == (kDNSRecordTypeUnique | kDNSRecordTypeVerified))
#define RecordIsLocalDuplicate(A,B) ((A)->resrec.InterfaceID == (B)->resrec.InterfaceID && RecordLDT((A),(B)) && IdenticalResourceRecord(&(A)->resrec, &(B)->resrec))

mDNSlocal mStatus mDNS_Register_internal(mDNS *const m, AuthRecord *const rr)
{
	domainname *target = GetRRDomainNameTarget(&rr->resrec);
	AuthRecord *r;
	AuthRecord **p = &m->ResourceRecords;
	AuthRecord **d = &m->DuplicateRecords;
	AuthRecord **l = &m->LocalOnlyRecords;
	
#if TEST_LOCALONLY_FOR_EVERYTHING
	rr->resrec.InterfaceID = (mDNSInterfaceID)~0;
#endif

	while (*p && *p != rr) p=&(*p)->next;
	while (*d && *d != rr) d=&(*d)->next;
	while (*l && *l != rr) l=&(*l)->next;
	if (*d || *p || *l)
	{
		LogMsg("Error! Tried to register a AuthRecord %p %##s (%s) that's already in the list", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
		return(mStatus_AlreadyRegistered);
	}

	if (rr->DependentOn)
	{
		if (rr->resrec.RecordType == kDNSRecordTypeUnique)
			rr->resrec.RecordType =  kDNSRecordTypeVerified;
		else
		{
			LogMsg("mDNS_Register_internal: ERROR! %##s (%s): rr->DependentOn && RecordType != kDNSRecordTypeUnique",
			       rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
			return(mStatus_Invalid);
		}
		if (!(rr->DependentOn->resrec.RecordType & (kDNSRecordTypeUnique | kDNSRecordTypeVerified)))
		{
			LogMsg("mDNS_Register_internal: ERROR! %##s (%s): rr->DependentOn->RecordType bad type %X",
			       rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), rr->DependentOn->resrec.RecordType);
			return(mStatus_Invalid);
		}
	}

	// If this resource record is referencing a specific interface, make sure it exists
	if (rr->resrec.InterfaceID && rr->resrec.InterfaceID != ((mDNSInterfaceID)~0))
	{
		NetworkInterfaceInfo *intf;
		for (intf = m->HostInterfaces; intf; intf = intf->next)
			if (intf->InterfaceID == rr->resrec.InterfaceID) break;
		if (!intf)
		{
			debugf("mDNS_Register_internal: Bogus InterfaceID %p in resource record", rr->resrec.InterfaceID);
			return(mStatus_BadReferenceErr);
		}
	}

	rr->next = mDNSNULL;

	// Field Group 1: Persistent metadata for Authoritative Records
//	rr->Additional1       = set to mDNSNULL in mDNS_SetupResourceRecord; may be overridden by client
//	rr->Additional2       = set to mDNSNULL in mDNS_SetupResourceRecord; may be overridden by client
//	rr->DependentOn       = set to mDNSNULL in mDNS_SetupResourceRecord; may be overridden by client
//	rr->RRSet             = set to mDNSNULL in mDNS_SetupResourceRecord; may be overridden by client
//	rr->Callback          = already set in mDNS_SetupResourceRecord
//	rr->Context           = already set in mDNS_SetupResourceRecord
//	rr->RecordType        = already set in mDNS_SetupResourceRecord
//	rr->HostTarget        = set to mDNSfalse in mDNS_SetupResourceRecord; may be overridden by client

	// Field Group 2: Transient state for Authoritative Records
	rr->Acknowledged      = mDNSfalse;
	rr->ProbeCount        = DefaultProbeCountForRecordType(rr->resrec.RecordType);
	rr->AnnounceCount     = InitialAnnounceCount;
	rr->IncludeInProbe    = mDNSfalse;
	rr->ImmedAnswer       = mDNSNULL;
	rr->ImmedAdditional   = mDNSNULL;
	rr->SendRNow          = mDNSNULL;
	rr->v4Requester       = zeroIPAddr;
	rr->v6Requester       = zerov6Addr;
	rr->NextResponse      = mDNSNULL;
	rr->NR_AnswerTo       = mDNSNULL;
	rr->NR_AdditionalTo   = mDNSNULL;
	rr->ThisAPInterval    = DefaultAPIntervalForRecordType(rr->resrec.RecordType);
	InitializeLastAPTime(m, rr);
//	rr->AnnounceUntil     = Set for us in InitializeLastAPTime()
//	rr->LastAPTime        = Set for us in InitializeLastAPTime()
//	rr->LastMCTime        = Set for us in InitializeLastAPTime()
//	rr->LastMCInterface   = Set for us in InitializeLastAPTime()
	rr->NewRData          = mDNSNULL;
	rr->newrdlength       = 0;
	rr->UpdateCallback    = mDNSNULL;
	rr->UpdateCredits     = kMaxUpdateCredits;
	rr->NextUpdateCredit  = 0;
	rr->UpdateBlocked     = 0;

//	rr->resrec.interface         = already set in mDNS_SetupResourceRecord
//	rr->resrec.name.c            = MUST be set by client
//	rr->resrec.rrtype            = already set in mDNS_SetupResourceRecord
//	rr->resrec.rrclass           = already set in mDNS_SetupResourceRecord
//	rr->resrec.rroriginalttl     = already set in mDNS_SetupResourceRecord
//	rr->resrec.rdata             = MUST be set by client, unless record type is CNAME or PTR and rr->HostTarget is set

	if (rr->HostTarget)
	{
		if (target) target->c[0] = 0;
		SetTargetToHostName(m, rr);	// This also sets rdlength and rdestimate for us
	}
	else
	{
		rr->resrec.rdlength   = GetRDLength(&rr->resrec, mDNSfalse);
		rr->resrec.rdestimate = GetRDLength(&rr->resrec, mDNStrue);
	}

	if (!ValidateDomainName(&rr->resrec.name))
	{ LogMsg("Attempt to register record with invalid name: %s", GetRRDisplayString(m, rr)); return(mStatus_Invalid); }

	// Don't do this until *after* we've set rr->resrec.rdlength
	if (!ValidateRData(rr->resrec.rrtype, rr->resrec.rdlength, rr->resrec.rdata))
	{ LogMsg("Attempt to register record with invalid rdata: %s", GetRRDisplayString(m, rr)); return(mStatus_Invalid); }

	rr->resrec.namehash   = DomainNameHashValue(&rr->resrec.name);
	rr->resrec.rdatahash  = RDataHashValue(rr->resrec.rdlength, &rr->resrec.rdata->u);
	rr->resrec.rdnamehash = target ? DomainNameHashValue(target) : 0;
	
	if (rr->resrec.InterfaceID == ((mDNSInterfaceID)~0))
	{
		debugf("Adding %p %##s (%s) to LocalOnly list", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
		*l = rr;
		if (!m->NewLocalOnlyRecords) m->NewLocalOnlyRecords = rr;
		// If this is supposed to be unique, make sure we don't have any name conflicts
		if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
		{
			const AuthRecord *s1 = rr->RRSet ? rr->RRSet : rr;
			for (r = m->LocalOnlyRecords; r; r=r->next)
			{
				const AuthRecord *s2 = r->RRSet ? r->RRSet : r;
				if (s1 != s2 && SameResourceRecordSignature(&r->resrec, &rr->resrec) && !SameRData(&r->resrec, &rr->resrec))
					break;
			}
			if (r)	// If we found a conflict, set DiscardLocalOnlyRecords so we'll deliver the callback
			{
				debugf("Name conflict %p %##s (%s)", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
				m->DiscardLocalOnlyRecords = mDNStrue;
			}
			else	// else no conflict, so set ProbeCount to zero and update RecordType as appropriate
			{
				rr->ProbeCount = 0;
				if (rr->resrec.RecordType == kDNSRecordTypeUnique) rr->resrec.RecordType = kDNSRecordTypeVerified;
			}
		}
	}
	else
	{
		// Now that's we've finished building our new record, make sure it's not identical to one we already have
		for (r = m->ResourceRecords; r; r=r->next) if (RecordIsLocalDuplicate(r, rr)) break;
		
		if (r)
		{
			debugf("Adding %p %##s (%s) to duplicate list", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
			*d = rr;
			// If the previous copy of this record is already verified unique,
			// then indicate that we should move this record promptly to kDNSRecordTypeUnique state.
			// Setting ProbeCount to zero will cause SendQueries() to advance this record to
			// kDNSRecordTypeVerified state and call the client callback at the next appropriate time.
			if (rr->resrec.RecordType == kDNSRecordTypeUnique && r->resrec.RecordType == kDNSRecordTypeVerified)
				rr->ProbeCount = 0;
		}
		else
		{
			debugf("Adding %p %##s (%s) to active record list", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
			*p = rr;
		}
	}
	return(mStatus_NoError);
}

mDNSlocal void RecordProbeFailure(mDNS *const m, const AuthRecord *const rr)
{
	m->ProbeFailTime = m->timenow;
	m->NumFailedProbes++;
	// If we've had ten or more probe failures, rate-limit to one every five seconds
	// The result is ORed with 1 to make sure SuppressProbes is not accidentally set to zero
	if (m->NumFailedProbes >= 10) m->SuppressProbes = (m->timenow + mDNSPlatformOneSecond * 5) | 1;
	if (m->NumFailedProbes >= 16)
		LogMsg("Name in use: %##s (%s); need to choose another (%d)",
		       rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), m->NumFailedProbes);
}

// mDNS_Dereg_normal is used for most calls to mDNS_Deregister_internal
// mDNS_Dereg_conflict is used to indicate that this record is being forcibly deregistered because of a conflict
// mDNS_Dereg_repeat is used when cleaning up, for records that may have already been forcibly deregistered
typedef enum { mDNS_Dereg_normal, mDNS_Dereg_conflict, mDNS_Dereg_repeat } mDNS_Dereg_type;

// NOTE: mDNS_Deregister_internal can call a user callback, which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal mStatus mDNS_Deregister_internal(mDNS *const m, AuthRecord *const rr, mDNS_Dereg_type drt)
{
	mDNSu8 RecordType = rr->resrec.RecordType;
	AuthRecord **p = &m->ResourceRecords;	// Find this record in our list of active records
	if (rr->resrec.InterfaceID == ((mDNSInterfaceID)~0)) p = &m->LocalOnlyRecords;
	while (*p && *p != rr) p=&(*p)->next;

	if (*p)
	{
		// We found our record on the main list. See if there are any duplicates that need special handling.
		if (drt == mDNS_Dereg_conflict)		// If this was a conflict, see that all duplicates get the same treatment
		{
			AuthRecord *r2 = m->DuplicateRecords;
			while (r2)
			{
				if (RecordIsLocalDuplicate(r2, rr)) { mDNS_Deregister_internal(m, r2, drt); r2 = m->DuplicateRecords; }
				else r2=r2->next;
			}
		}
		else
		{
			// Before we delete the record (and potentially send a goodbye packet)
			// first see if we have a record on the duplicate list ready to take over from it.
			AuthRecord **d = &m->DuplicateRecords;
			while (*d && !RecordIsLocalDuplicate(*d, rr)) d=&(*d)->next;
			if (*d)
			{
				AuthRecord *dup = *d;
				debugf("Duplicate record %p taking over from %p %##s (%s)", dup, rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
				*d        = dup->next;		// Cut replacement record from DuplicateRecords list
				dup->next = rr->next;		// And then...
				rr->next  = dup;			// ... splice it in right after the record we're about to delete
				dup->resrec.RecordType        = rr->resrec.RecordType;
				dup->ProbeCount        = rr->ProbeCount;
				dup->AnnounceCount     = rr->AnnounceCount;
				dup->ImmedAnswer       = rr->ImmedAnswer;
				dup->ImmedAdditional   = rr->ImmedAdditional;
				dup->v4Requester       = rr->v4Requester;
				dup->v6Requester       = rr->v6Requester;
				dup->ThisAPInterval    = rr->ThisAPInterval;
				dup->AnnounceUntil     = rr->AnnounceUntil;
				dup->LastAPTime        = rr->LastAPTime;
				dup->LastMCTime        = rr->LastMCTime;
				dup->LastMCInterface   = rr->LastMCInterface;
				if (RecordType == kDNSRecordTypeShared) rr->AnnounceCount = InitialAnnounceCount;
			}
		}
	}
	else
	{
		// We didn't find our record on the main list; try the DuplicateRecords list instead.
		p = &m->DuplicateRecords;
		while (*p && *p != rr) p=&(*p)->next;
		// If we found our record on the duplicate list, then make sure we don't send a goodbye for it
		if (*p && RecordType == kDNSRecordTypeShared) rr->AnnounceCount = InitialAnnounceCount;
		if (*p) debugf("DNS_Deregister_internal: Deleting DuplicateRecord %p %##s (%s)", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
	}

	if (!*p)
	{
		// No need to log an error message if we already know this is a potentially repeated deregistration
		if (drt != mDNS_Dereg_repeat)
			debugf("mDNS_Deregister_internal: Record %p %##s (%s) not found in list", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
		return(mStatus_BadReferenceErr);
	}

	// If this is a shared record and we've announced it at least once,
	// we need to retract that announcement before we delete the record
	if (RecordType == kDNSRecordTypeShared && rr->AnnounceCount < InitialAnnounceCount)
	{
		verbosedebugf("mDNS_Deregister_internal: Sending deregister for %##s (%s)", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
		rr->resrec.RecordType    = kDNSRecordTypeDeregistering;
		rr->resrec.rroriginalttl = 0;
		rr->ImmedAnswer          = mDNSInterfaceMark;
		if (rr->resrec.InterfaceID == ((mDNSInterfaceID)~0))
			m->DiscardLocalOnlyRecords = mDNStrue;
		else
		{
			if (m->NextScheduledResponse - (m->timenow + mDNSPlatformOneSecond/10) >= 0)
				m->NextScheduledResponse = (m->timenow + mDNSPlatformOneSecond/10);
		}
	}
	else
	{
		*p = rr->next;					// Cut this record from the list
		// If someone is about to look at this, bump the pointer forward
		if (m->CurrentRecord       == rr) m->CurrentRecord       = rr->next;
		if (m->NewLocalOnlyRecords == rr) m->NewLocalOnlyRecords = rr->next;
		rr->next = mDNSNULL;

		if      (RecordType == kDNSRecordTypeUnregistered)
			debugf("mDNS_Deregister_internal: Record %##s (%s) already marked kDNSRecordTypeUnregistered",
			       rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
		else if (RecordType == kDNSRecordTypeDeregistering)
			debugf("mDNS_Deregister_internal: Record %##s (%s) already marked kDNSRecordTypeDeregistering",
			       rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
		else
		{
			verbosedebugf("mDNS_Deregister_internal: Deleting record for %##s (%s)", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
			rr->resrec.RecordType = kDNSRecordTypeUnregistered;
		}

		if ((drt == mDNS_Dereg_conflict || drt == mDNS_Dereg_repeat) && RecordType == kDNSRecordTypeShared)
			debugf("mDNS_Deregister_internal: Cannot have a conflict on a shared record! %##s (%s)",
			       rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));

		// If we have an update queued up which never executed, give the client a chance to free that memory
		if (rr->NewRData)
		{
			RData *OldRData = rr->resrec.rdata;
			SetNewRData(&rr->resrec, rr->NewRData, rr->newrdlength);	// Update our rdata
			rr->NewRData = mDNSNULL;									// Clear the NewRData pointer ...
			if (rr->UpdateCallback)
				rr->UpdateCallback(m, rr, OldRData);					// ... and let the client know
		}
		
		// CAUTION: MUST NOT do anything more with rr after calling rr->Callback(), because the client's callback function
		// is allowed to do anything, including starting/stopping queries, registering/deregistering records, etc.
		// In this case the likely client action to the mStatus_MemFree message is to free the memory,
		// so any attempt to touch rr after this is likely to lead to a crash.
		m->mDNS_reentrancy++; // Increment to allow client to legally make mDNS API calls from the callback
		if (RecordType == kDNSRecordTypeShared)
		{
			if (rr->RecordCallback)
				rr->RecordCallback(m, rr, mStatus_MemFree);
		}
		else if (drt == mDNS_Dereg_conflict)
		{
			RecordProbeFailure(m, rr);
			if (rr->RecordCallback)
				rr->RecordCallback(m, rr, mStatus_NameConflict);
		}
		m->mDNS_reentrancy--; // Decrement to block mDNS API calls again
	}
	return(mStatus_NoError);
}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark -
#pragma mark - DNS Message Creation Functions
#endif

mDNSlocal void InitializeDNSMessage(DNSMessageHeader *h, mDNSOpaque16 id, mDNSOpaque16 flags)
{
	h->id             = id;
	h->flags          = flags;
	h->numQuestions   = 0;
	h->numAnswers     = 0;
	h->numAuthorities = 0;
	h->numAdditionals = 0;
}

mDNSlocal const mDNSu8 *FindCompressionPointer(const mDNSu8 *const base, const mDNSu8 *const end, const mDNSu8 *const domname)
{
	const mDNSu8 *result = end - *domname - 1;

	if (*domname == 0) return(mDNSNULL);	// There's no point trying to match just the root label
	
	// This loop examines each possible starting position in packet, starting end of the packet and working backwards
	while (result >= base)
	{
		// If the length byte and first character of the label match, then check further to see
		// if this location in the packet will yield a useful name compression pointer.
		if (result[0] == domname[0] && result[1] == domname[1])
		{
			const mDNSu8 *name = domname;
			const mDNSu8 *targ = result;
			while (targ + *name < end)
			{
				// First see if this label matches
				int i;
				const mDNSu8 *pointertarget;
				for (i=0; i <= *name; i++) if (targ[i] != name[i]) break;
				if (i <= *name) break;							// If label did not match, bail out
				targ += 1 + *name;								// Else, did match, so advance target pointer
				name += 1 + *name;								// and proceed to check next label
				if (*name == 0 && *targ == 0) return(result);	// If no more labels, we found a match!
				if (*name == 0) break;							// If no more labels to match, we failed, so bail out

				// The label matched, so now follow the pointer (if appropriate) and then see if the next label matches
				if (targ[0] < 0x40) continue;					// If length value, continue to check next label
				if (targ[0] < 0xC0) break;						// If 40-BF, not valid
				if (targ+1 >= end) break;						// Second byte not present!
				pointertarget = base + (((mDNSu16)(targ[0] & 0x3F)) << 8) + targ[1];
				if (targ < pointertarget) break;				// Pointertarget must point *backwards* in the packet
				if (pointertarget[0] >= 0x40) break;			// Pointertarget must point to a valid length byte
				targ = pointertarget;
			}
		}
		result--;	// We failed to match at this search position, so back up the tentative result pointer and try again
	}
	return(mDNSNULL);
}

// Put a string of dot-separated labels as length-prefixed labels
// domainname is a fully-qualified name (i.e. assumed to be ending in a dot, even if it doesn't)
// msg points to the message we're building (pass mDNSNULL if we don't want to use compression pointers)
// end points to the end of the message so far
// ptr points to where we want to put the name
// limit points to one byte past the end of the buffer that we must not overrun
// domainname is the name to put
mDNSlocal mDNSu8 *putDomainNameAsLabels(const DNSMessage *const msg,
					mDNSu8 *ptr, const mDNSu8 *const limit, const domainname *const name)
{
	const mDNSu8 *const base        = (const mDNSu8 *)msg;
	const mDNSu8 *      np          = name->c;
	const mDNSu8 *const max         = name->c + MAX_DOMAIN_NAME;	// Maximum that's valid
	const mDNSu8 *      pointer     = mDNSNULL;
	const mDNSu8 *const searchlimit = ptr;

	while (*np && ptr < limit-1)		// While we've got characters in the name, and space to write them in the message...
	{
		if (*np > MAX_DOMAIN_LABEL)
		{ LogMsg("Malformed domain name %##s (label more than 63 bytes)", name->c); return(mDNSNULL); }
		
		// This check correctly allows for the final trailing root label:
		// e.g.
		// Suppose our domain name is exactly 255 bytes long, including the final trailing root label.
		// Suppose np is now at name->c[248], and we're about to write our last non-null label ("local").
		// We know that max will be at name->c[255]
		// That means that np + 1 + 5 == max - 1, so we (just) pass the "if" test below, write our
		// six bytes, then exit the loop, write the final terminating root label, and the domain
		// name we've written is exactly 255 bytes long, exactly at the correct legal limit.
		// If the name is one byte longer, then we fail the "if" test below, and correctly bail out.
		if (np + 1 + *np >= max)
		{ LogMsg("Malformed domain name %##s (more than 255 bytes)", name->c); return(mDNSNULL); }

		if (base) pointer = FindCompressionPointer(base, searchlimit, np);
		if (pointer)					// Use a compression pointer if we can
		{
			mDNSu16 offset = (mDNSu16)(pointer - base);
			*ptr++ = (mDNSu8)(0xC0 | (offset >> 8));
			*ptr++ = (mDNSu8)(        offset      );
			return(ptr);
		}
		else							// Else copy one label and try again
		{
			int i;
			mDNSu8 len = *np++;
			if (ptr + 1 + len >= limit) return(mDNSNULL);
			*ptr++ = len;
			for (i=0; i<len; i++) *ptr++ = *np++;
		}
	}

	if (ptr < limit)												// If we didn't run out of space
	{
		*ptr++ = 0;													// Put the final root label
		return(ptr);												// and return
	}

	return(mDNSNULL);
}

mDNSlocal mDNSu8 *putRData(const DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit, ResourceRecord *rr)
{
	switch (rr->rrtype)
	{
	case kDNSType_A:	if (rr->rdlength != 4)
	{
		debugf("putRData: Illegal length %d for kDNSType_A", rr->rdlength);
		return(mDNSNULL);
	}
		if (ptr + 4 > limit) return(mDNSNULL);
		*ptr++ = rr->rdata->u.ip.b[0];
		*ptr++ = rr->rdata->u.ip.b[1];
		*ptr++ = rr->rdata->u.ip.b[2];
		*ptr++ = rr->rdata->u.ip.b[3];
		return(ptr);

	case kDNSType_CNAME:// Same as PTR
	case kDNSType_PTR:	return(putDomainNameAsLabels(msg, ptr, limit, &rr->rdata->u.name));

	case kDNSType_HINFO:// Same as TXT
	case kDNSType_TXT:  if (ptr + rr->rdlength > limit) return(mDNSNULL);
		mDNSPlatformMemCopy(rr->rdata->u.data, ptr, rr->rdlength);
		return(ptr + rr->rdlength);

	case kDNSType_AAAA:	if (rr->rdlength != sizeof(rr->rdata->u.ipv6))
	{
		debugf("putRData: Illegal length %d for kDNSType_AAAA", rr->rdlength);
		return(mDNSNULL);
	}
		if (ptr + sizeof(rr->rdata->u.ipv6) > limit) return(mDNSNULL);
		mDNSPlatformMemCopy(&rr->rdata->u.ipv6, ptr, sizeof(rr->rdata->u.ipv6));
		return(ptr + sizeof(rr->rdata->u.ipv6));

	case kDNSType_SRV:	if (ptr + 6 > limit) return(mDNSNULL);
		*ptr++ = (mDNSu8)(rr->rdata->u.srv.priority >> 8);
		*ptr++ = (mDNSu8)(rr->rdata->u.srv.priority     );
		*ptr++ = (mDNSu8)(rr->rdata->u.srv.weight   >> 8);
		*ptr++ = (mDNSu8)(rr->rdata->u.srv.weight       );
		*ptr++ = rr->rdata->u.srv.port.b[0];
		*ptr++ = rr->rdata->u.srv.port.b[1];
		return(putDomainNameAsLabels(msg, ptr, limit, &rr->rdata->u.srv.target));

	default:			if (ptr + rr->rdlength > limit) return(mDNSNULL);
		debugf("putRData: Warning! Writing resource type %d as raw data", rr->rrtype);
		mDNSPlatformMemCopy(rr->rdata->u.data, ptr, rr->rdlength);
		return(ptr + rr->rdlength);
	}
}

mDNSlocal mDNSu8 *PutResourceRecordTTL(DNSMessage *const msg, mDNSu8 *ptr, mDNSu16 *count, ResourceRecord *rr, mDNSu32 ttl)
{
	mDNSu8 *endofrdata;
	mDNSu16 actualLength;
	const mDNSu8 *limit = msg->data + AbsoluteMaxDNSMessageData;
	
	// If we have a single large record to put in the packet, then we allow the packet to be up to 9K bytes,
	// but in the normal case we try to keep the packets below 1500 to avoid IP fragmentation on standard Ethernet
	if (msg->h.numAnswers || msg->h.numAuthorities || msg->h.numAdditionals)
		limit = msg->data + NormalMaxDNSMessageData;

	if (rr->RecordType == kDNSRecordTypeUnregistered)
	{
		LogMsg("PutResourceRecord ERROR! Attempt to put kDNSRecordTypeUnregistered %##s (%s)", rr->name.c, DNSTypeName(rr->rrtype));
		return(ptr);
	}

	ptr = putDomainNameAsLabels(msg, ptr, limit, &rr->name);
	if (!ptr || ptr + 10 >= limit) return(mDNSNULL);	// If we're out-of-space, return mDNSNULL
	ptr[0] = (mDNSu8)(rr->rrtype  >> 8);
	ptr[1] = (mDNSu8)(rr->rrtype      );
	ptr[2] = (mDNSu8)(rr->rrclass >> 8);
	ptr[3] = (mDNSu8)(rr->rrclass     );
	ptr[4] = (mDNSu8)(ttl >> 24);
	ptr[5] = (mDNSu8)(ttl >> 16);
	ptr[6] = (mDNSu8)(ttl >>  8);
	ptr[7] = (mDNSu8)(ttl      );
	endofrdata = putRData(msg, ptr+10, limit, rr);
	if (!endofrdata) { verbosedebugf("Ran out of space in PutResourceRecord for %##s (%s)", rr->name.c, DNSTypeName(rr->rrtype)); return(mDNSNULL); }

	// Go back and fill in the actual number of data bytes we wrote
	// (actualLength can be less than rdlength when domain name compression is used)
	actualLength = (mDNSu16)(endofrdata - ptr - 10);
	ptr[8] = (mDNSu8)(actualLength >> 8);
	ptr[9] = (mDNSu8)(actualLength     );

	(*count)++;
	return(endofrdata);
}

#define PutResourceRecord(MSG, P, C, RR) PutResourceRecordTTL((MSG), (P), (C), (RR), (RR)->rroriginalttl)

mDNSlocal mDNSu8 *PutResourceRecordCappedTTL(DNSMessage *const msg, mDNSu8 *ptr, mDNSu16 *count, ResourceRecord *rr, mDNSu32 maxttl)
{
	if (maxttl > rr->rroriginalttl) maxttl = rr->rroriginalttl;
	return(PutResourceRecordTTL(msg, ptr, count, rr, maxttl));
}

#if 0
mDNSlocal mDNSu8 *putEmptyResourceRecord(DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit,
					 mDNSu16 *count, const AuthRecord *rr)
{
	ptr = putDomainNameAsLabels(msg, ptr, limit, &rr->name);
	if (!ptr || ptr + 10 > limit) return(mDNSNULL);		// If we're out-of-space, return mDNSNULL
	ptr[0] = (mDNSu8)(rr->resrec.rrtype  >> 8);				// Put type
	ptr[1] = (mDNSu8)(rr->resrec.rrtype      );
	ptr[2] = (mDNSu8)(rr->resrec.rrclass >> 8);				// Put class
	ptr[3] = (mDNSu8)(rr->resrec.rrclass     );
	ptr[4] = ptr[5] = ptr[6] = ptr[7] = 0;				// TTL is zero
	ptr[8] = ptr[9] = 0;								// RDATA length is zero
	(*count)++;
	return(ptr + 10);
}
#endif

mDNSlocal mDNSu8 *putQuestion(DNSMessage *const msg, mDNSu8 *ptr, const mDNSu8 *const limit,
			      const domainname *const name, mDNSu16 rrtype, mDNSu16 rrclass)
{
	ptr = putDomainNameAsLabels(msg, ptr, limit, name);
	if (!ptr || ptr+4 >= limit) return(mDNSNULL);			// If we're out-of-space, return mDNSNULL
	ptr[0] = (mDNSu8)(rrtype  >> 8);
	ptr[1] = (mDNSu8)(rrtype      );
	ptr[2] = (mDNSu8)(rrclass >> 8);
	ptr[3] = (mDNSu8)(rrclass     );
	msg->h.numQuestions++;
	return(ptr+4);
}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - DNS Message Parsing Functions
#endif

mDNSlocal const mDNSu8 *skipDomainName(const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *const end)
{
	mDNSu16 total = 0;

	if (ptr < (mDNSu8*)msg || ptr >= end)
	{ debugf("skipDomainName: Illegal ptr not within packet boundaries"); return(mDNSNULL); }

	while (1)						// Read sequence of labels
	{
		const mDNSu8 len = *ptr++;	// Read length of this label
		if (len == 0) return(ptr);	// If length is zero, that means this name is complete
		switch (len & 0xC0)
		{
		case 0x00:	if (ptr + len >= end)					// Remember: expect at least one more byte for the root label
		{ debugf("skipDomainName: Malformed domain name (overruns packet end)"); return(mDNSNULL); }
			if (total + 1 + len >= MAX_DOMAIN_NAME)	// Remember: expect at least one more byte for the root label
			{ debugf("skipDomainName: Malformed domain name (more than 255 characters)"); return(mDNSNULL); }
			ptr += len;
			total += 1 + len;
			break;

		case 0x40:	debugf("skipDomainName: Extended EDNS0 label types 0x%X not supported", len); return(mDNSNULL);
		case 0x80:	debugf("skipDomainName: Illegal label length 0x%X", len); return(mDNSNULL);
		case 0xC0:	return(ptr+1);
		}
	}
}

// Routine to fetch an FQDN from the DNS message, following compression pointers if necessary.
mDNSlocal const mDNSu8 *getDomainName(const DNSMessage *const msg, const mDNSu8 *ptr, const mDNSu8 *const end,
				      domainname *const name)
{
	const mDNSu8 *nextbyte = mDNSNULL;					// Record where we got to before we started following pointers
	mDNSu8       *np = name->c;							// Name pointer
	const mDNSu8 *const limit = np + MAX_DOMAIN_NAME;	// Limit so we don't overrun buffer

	if (ptr < (mDNSu8*)msg || ptr >= end)
	{ debugf("getDomainName: Illegal ptr not within packet boundaries"); return(mDNSNULL); }

	*np = 0;						// Tentatively place the root label here (may be overwritten if we have more labels)

	while (1)						// Read sequence of labels
	{
		const mDNSu8 len = *ptr++;	// Read length of this label
		if (len == 0) break;		// If length is zero, that means this name is complete
		switch (len & 0xC0)
		{
			int i;
			mDNSu16 offset;

		case 0x00:	if (ptr + len >= end)		// Remember: expect at least one more byte for the root label
		{ debugf("getDomainName: Malformed domain name (overruns packet end)"); return(mDNSNULL); }
		if (np + 1 + len >= limit)	// Remember: expect at least one more byte for the root label
		{ debugf("getDomainName: Malformed domain name (more than 255 characters)"); return(mDNSNULL); }
		*np++ = len;
		for (i=0; i<len; i++) *np++ = *ptr++;
		*np = 0;	// Tentatively place the root label here (may be overwritten if we have more labels)
		break;

		case 0x40:	debugf("getDomainName: Extended EDNS0 label types 0x%X not supported in name %##s", len, name->c);
		return(mDNSNULL);

		case 0x80:	debugf("getDomainName: Illegal label length 0x%X in domain name %##s", len, name->c); return(mDNSNULL);

		case 0xC0:	offset = (mDNSu16)((((mDNSu16)(len & 0x3F)) << 8) | *ptr++);
			if (!nextbyte) nextbyte = ptr;	// Record where we got to before we started following pointers
			ptr = (mDNSu8 *)msg + offset;
			if (ptr < (mDNSu8*)msg || ptr >= end)
			{ debugf("getDomainName: Illegal compression pointer not within packet boundaries"); return(mDNSNULL); }
			if (*ptr & 0xC0)
			{ debugf("getDomainName: Compression pointer must point to real label"); return(mDNSNULL); }
			break;
		}
	}
	
	if (nextbyte) return(nextbyte);
	else return(ptr);
}

mDNSlocal const mDNSu8 *skipResourceRecord(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end)
{
	mDNSu16 pktrdlength;

	ptr = skipDomainName(msg, ptr, end);
	if (!ptr) { debugf("skipResourceRecord: Malformed RR name"); return(mDNSNULL); }
	
	if (ptr + 10 > end) { debugf("skipResourceRecord: Malformed RR -- no type/class/ttl/len!"); return(mDNSNULL); }
	pktrdlength = (mDNSu16)((mDNSu16)ptr[8] <<  8 | ptr[9]);
	ptr += 10;
	if (ptr + pktrdlength > end) { debugf("skipResourceRecord: RDATA exceeds end of packet"); return(mDNSNULL); }

	return(ptr + pktrdlength);
}

#define GetLargeResourceRecord(m, msg, p, e, i, t, L) \
	(((L)->r.rdatastorage.MaxRDLength = MaximumRDSize), GetResourceRecord((m), (msg), (p), (e), (i), (t), &(L)->r, (RData*)&(L)->r.rdatastorage))

mDNSlocal const mDNSu8 *GetResourceRecord(mDNS *const m, const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end,
					  const mDNSInterfaceID InterfaceID, mDNSu8 RecordType, CacheRecord *rr, RData *RDataStorage)
{
	mDNSu16 pktrdlength;

	rr->next              = mDNSNULL;
	rr->resrec.RecordType        = RecordType;

	rr->NextInKAList      = mDNSNULL;
	rr->TimeRcvd          = m->timenow;
	rr->NextRequiredQuery = m->timenow;		// Will be updated to the real value when we call SetNextCacheCheckTime()
	rr->LastUsed          = m->timenow;
	rr->UseCount          = 0;
	rr->CRActiveQuestion  = mDNSNULL;
	rr->UnansweredQueries = 0;
	rr->LastUnansweredTime= 0;
	rr->MPUnansweredQ     = 0;
	rr->MPLastUnansweredQT= 0;
	rr->MPUnansweredKA    = 0;
	rr->MPExpectingKA     = mDNSfalse;
	rr->NextInCFList      = mDNSNULL;

	rr->resrec.InterfaceID       = InterfaceID;
	ptr = getDomainName(msg, ptr, end, &rr->resrec.name);
	if (!ptr) { debugf("GetResourceRecord: Malformed RR name"); return(mDNSNULL); }

	if (ptr + 10 > end) { debugf("GetResourceRecord: Malformed RR -- no type/class/ttl/len!"); return(mDNSNULL); }
	
	rr->resrec.rrtype            = (mDNSu16) ((mDNSu16)ptr[0] <<  8 | ptr[1]);
	rr->resrec.rrclass           = (mDNSu16)(((mDNSu16)ptr[2] <<  8 | ptr[3]) & kDNSClass_Mask);
	rr->resrec.rroriginalttl     = (mDNSu32) ((mDNSu32)ptr[4] << 24 | (mDNSu32)ptr[5] << 16 | (mDNSu32)ptr[6] << 8 | ptr[7]);
	if (rr->resrec.rroriginalttl > 0x70000000UL / mDNSPlatformOneSecond)
		rr->resrec.rroriginalttl = 0x70000000UL / mDNSPlatformOneSecond;
	// Note: We don't have to adjust m->NextCacheCheck here -- this is just getting a record into memory for
	// us to look at. If we decide to copy it into the cache, then we'll update m->NextCacheCheck accordingly.
	pktrdlength           = (mDNSu16)((mDNSu16)ptr[8] <<  8 | ptr[9]);
	if (ptr[2] & (kDNSClass_UniqueRRSet >> 8))
		rr->resrec.RecordType |= kDNSRecordTypePacketUniqueMask;
	ptr += 10;
	if (ptr + pktrdlength > end) { debugf("GetResourceRecord: RDATA exceeds end of packet"); return(mDNSNULL); }

	if (RDataStorage)
		rr->resrec.rdata = RDataStorage;
	else
	{
		rr->resrec.rdata = (RData*)&rr->rdatastorage;
		rr->resrec.rdata->MaxRDLength = sizeof(RDataBody);
	}

	switch (rr->resrec.rrtype)
	{
	case kDNSType_A:	rr->resrec.rdata->u.ip.b[0] = ptr[0];
		rr->resrec.rdata->u.ip.b[1] = ptr[1];
		rr->resrec.rdata->u.ip.b[2] = ptr[2];
		rr->resrec.rdata->u.ip.b[3] = ptr[3];
		break;

	case kDNSType_CNAME:// Same as PTR
	case kDNSType_PTR:	if (!getDomainName(msg, ptr, end, &rr->resrec.rdata->u.name))
	{ debugf("GetResourceRecord: Malformed CNAME/PTR RDATA name"); return(mDNSNULL); }
		//debugf("%##s PTR %##s rdlen %d", rr->resrec.name.c, rr->resrec.rdata->u.name.c, pktrdlength);
		break;

	case kDNSType_NULL:	//Same as TXT
	case kDNSType_HINFO://Same as TXT
	case kDNSType_TXT:  if (pktrdlength > rr->resrec.rdata->MaxRDLength)
	{
		debugf("GetResourceRecord: %s rdata size (%d) exceeds storage (%d)",
		       DNSTypeName(rr->resrec.rrtype), pktrdlength, rr->resrec.rdata->MaxRDLength);
		return(mDNSNULL);
	}
		rr->resrec.rdlength = pktrdlength;
		mDNSPlatformMemCopy(ptr, rr->resrec.rdata->u.data, pktrdlength);
		break;

	case kDNSType_AAAA:	mDNSPlatformMemCopy(ptr, &rr->resrec.rdata->u.ipv6, sizeof(rr->resrec.rdata->u.ipv6));
		break;

	case kDNSType_SRV:	rr->resrec.rdata->u.srv.priority = (mDNSu16)((mDNSu16)ptr[0] <<  8 | ptr[1]);
		rr->resrec.rdata->u.srv.weight   = (mDNSu16)((mDNSu16)ptr[2] <<  8 | ptr[3]);
		rr->resrec.rdata->u.srv.port.b[0] = ptr[4];
		rr->resrec.rdata->u.srv.port.b[1] = ptr[5];
		if (!getDomainName(msg, ptr+6, end, &rr->resrec.rdata->u.srv.target))
		{ debugf("GetResourceRecord: Malformed SRV RDATA name"); return(mDNSNULL); }
		//debugf("%##s SRV %##s rdlen %d", rr->resrec.name.c, rr->resrec.rdata->u.srv.target.c, pktrdlength);
		break;

	default:			if (pktrdlength > rr->resrec.rdata->MaxRDLength)
	{
		debugf("GetResourceRecord: rdata %d (%s) size (%d) exceeds storage (%d)",
		       rr->resrec.rrtype, DNSTypeName(rr->resrec.rrtype), pktrdlength, rr->resrec.rdata->MaxRDLength);
		return(mDNSNULL);
	}
		debugf("GetResourceRecord: Warning! Reading resource type %d (%s) as opaque data",
		       rr->resrec.rrtype, DNSTypeName(rr->resrec.rrtype));
		// Note: Just because we don't understand the record type, that doesn't
		// mean we fail. The DNS protocol specifies rdlength, so we can
		// safely skip over unknown records and ignore them.
		// We also grab a binary copy of the rdata anyway, since the caller
		// might know how to interpret it even if we don't.
		rr->resrec.rdlength = pktrdlength;
		mDNSPlatformMemCopy(ptr, rr->resrec.rdata->u.data, pktrdlength);
		break;
	}

	rr->resrec.namehash = DomainNameHashValue(&rr->resrec.name);
	SetNewRData(&rr->resrec, mDNSNULL, 0);

	return(ptr + pktrdlength);
}

mDNSlocal const mDNSu8 *skipQuestion(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end)
{
	ptr = skipDomainName(msg, ptr, end);
	if (!ptr) { debugf("skipQuestion: Malformed domain name in DNS question section"); return(mDNSNULL); }
	if (ptr+4 > end) { debugf("skipQuestion: Malformed DNS question section -- no query type and class!"); return(mDNSNULL); }
	return(ptr+4);
}

mDNSlocal const mDNSu8 *getQuestion(const DNSMessage *msg, const mDNSu8 *ptr, const mDNSu8 *end, const mDNSInterfaceID InterfaceID,
				    DNSQuestion *question)
{
	question->InterfaceID = InterfaceID;
	ptr = getDomainName(msg, ptr, end, &question->qname);
	if (!ptr) { debugf("Malformed domain name in DNS question section"); return(mDNSNULL); }
	if (ptr+4 > end) { debugf("Malformed DNS question section -- no query type and class!"); return(mDNSNULL); }

	question->qnamehash = DomainNameHashValue(&question->qname);
	question->qtype  = (mDNSu16)((mDNSu16)ptr[0] << 8 | ptr[1]);			// Get type
	question->qclass = (mDNSu16)((mDNSu16)ptr[2] << 8 | ptr[3]);			// and class
	return(ptr+4);
}

mDNSlocal const mDNSu8 *LocateAnswers(const DNSMessage *const msg, const mDNSu8 *const end)
{
	int i;
	const mDNSu8 *ptr = msg->data;
	for (i = 0; i < msg->h.numQuestions && ptr; i++) ptr = skipQuestion(msg, ptr, end);
	return(ptr);
}

mDNSlocal const mDNSu8 *LocateAuthorities(const DNSMessage *const msg, const mDNSu8 *const end)
{
	int i;
	const mDNSu8 *ptr = LocateAnswers(msg, end);
	for (i = 0; i < msg->h.numAnswers && ptr; i++) ptr = skipResourceRecord(msg, ptr, end);
	return(ptr);
}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark -
#pragma mark - Packet Sending Functions
#endif

mDNSlocal mStatus mDNSSendDNSMessage(const mDNS *const m, DNSMessage *const msg, const mDNSu8 *const end,
				     mDNSInterfaceID InterfaceID, mDNSIPPort srcport, const mDNSAddr *dst, mDNSIPPort dstport)
{
	mStatus status;
	mDNSu16 numQuestions   = msg->h.numQuestions;
	mDNSu16 numAnswers     = msg->h.numAnswers;
	mDNSu16 numAuthorities = msg->h.numAuthorities;
	mDNSu16 numAdditionals = msg->h.numAdditionals;
	
	// Put all the integer values in IETF byte-order (MSB first, LSB second)
	mDNSu8 *ptr = (mDNSu8 *)&msg->h.numQuestions;
	*ptr++ = (mDNSu8)(numQuestions   >> 8);
	*ptr++ = (mDNSu8)(numQuestions       );
	*ptr++ = (mDNSu8)(numAnswers     >> 8);
	*ptr++ = (mDNSu8)(numAnswers         );
	*ptr++ = (mDNSu8)(numAuthorities >> 8);
	*ptr++ = (mDNSu8)(numAuthorities     );
	*ptr++ = (mDNSu8)(numAdditionals >> 8);
	*ptr++ = (mDNSu8)(numAdditionals     );
	
	// Send the packet on the wire
	status = mDNSPlatformSendUDP(m, msg, end, InterfaceID, srcport, dst, dstport);
	
	// Put all the integer values back the way they were before we return
	msg->h.numQuestions   = numQuestions;
	msg->h.numAnswers     = numAnswers;
	msg->h.numAuthorities = numAuthorities;
	msg->h.numAdditionals = numAdditionals;

	return(status);
}

mDNSlocal void CompleteDeregistration(mDNS *const m, AuthRecord *rr)
{
	// Setting AnnounceCount to InitialAnnounceCount signals mDNS_Deregister_internal()
	// that it should go ahead and immediately dispose of this registration
	rr->resrec.RecordType    = kDNSRecordTypeShared;
	rr->AnnounceCount = InitialAnnounceCount;
	mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
}

// NOTE: DiscardDeregistrations calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void DiscardDeregistrations(mDNS *const m)
{
	if (m->CurrentRecord) LogMsg("DiscardDeregistrations ERROR m->CurrentRecord already set");
	m->CurrentRecord = m->ResourceRecords;
	
	while (m->CurrentRecord)
	{
		AuthRecord *rr = m->CurrentRecord;
		m->CurrentRecord = rr->next;
		if (rr->resrec.RecordType == kDNSRecordTypeDeregistering)
			CompleteDeregistration(m, rr);
	}
}

mDNSlocal mDNSBool HaveSentEntireRRSet(const mDNS *const m, const AuthRecord *const rr, mDNSInterfaceID InterfaceID)
{
	// Try to find another member of this set that we're still planning to send on this interface
	const AuthRecord *a;
	for (a = m->ResourceRecords; a; a=a->next)
		if (a->SendRNow == InterfaceID && a != rr && SameResourceRecordSignature(&a->resrec, &rr->resrec)) break;
	return (a == mDNSNULL);		// If no more members of this set found, then we should set the cache flush bit
}

// Note about acceleration of announcements to facilitate automatic coalescing of
// multiple independent threads of announcements into a single synchronized thread:
// The announcements in the packet may be at different stages of maturity;
// One-second interval, two-second interval, four-second interval, and so on.
// After we've put in all the announcements that are due, we then consider
// whether there are other nearly-due announcements that are worth accelerating.
// To be eligible for acceleration, a record MUST NOT be older (further along
// its timeline) than the most mature record we've already put in the packet.
// In other words, younger records can have their timelines accelerated to catch up
// with their elder bretheren; this narrows the age gap and helps them eventually get in sync.
// Older records cannot have their timelines accelerated; this would just widen
// the gap between them and their younger bretheren and get them even more out of sync.

// NOTE: SendResponses calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void SendResponses(mDNS *const m)
{
	int pktcount = 0;
	AuthRecord *rr, *r2;
	mDNSs32 maxExistingAnnounceInterval = 0;
	const NetworkInterfaceInfo *intf = GetFirstActiveInterface(m->HostInterfaces);

	m->NextScheduledResponse = m->timenow + kNextScheduledTime;

	// ***
	// *** 1. Setup: Set the SendRNow and ImmedAnswer fields to indicate which interface(s) the records need to be sent on
	// ***

	// Run through our list of records, and decide which ones we're going to announce on all interfaces
	for (rr = m->ResourceRecords; rr; rr=rr->next)
	{
		if (rr->NextUpdateCredit && m->timenow - rr->NextUpdateCredit >= 0)
		{
			if (++rr->UpdateCredits >= kMaxUpdateCredits) rr->NextUpdateCredit = 0; 
			else rr->NextUpdateCredit = (m->timenow + mDNSPlatformOneSecond * 60) | 1;
		}
		if (TimeToAnnounceThisRecord(rr, m->timenow) && ResourceRecordIsValidAnswer(rr))
		{
			rr->ImmedAnswer = mDNSInterfaceMark;		// Send on all interfaces
			if (maxExistingAnnounceInterval < rr->ThisAPInterval)
				maxExistingAnnounceInterval = rr->ThisAPInterval;
			if (rr->UpdateBlocked) rr->UpdateBlocked = 0;
		}
	}

	// Any interface-specific records we're going to send are marked as being sent on all appropriate interfaces (which is just one)
	// Eligible records that are more than half-way to their announcement time are accelerated
	for (rr = m->ResourceRecords; rr; rr=rr->next)
		if ((rr->resrec.InterfaceID && rr->ImmedAnswer) ||
		    (rr->ThisAPInterval <= maxExistingAnnounceInterval &&
		     TimeToAnnounceThisRecord(rr, m->timenow + rr->ThisAPInterval/2) &&
		     ResourceRecordIsValidAnswer(rr)))
			rr->ImmedAnswer = mDNSInterfaceMark;		// Send on all interfaces

	// When sending SRV records (particularly when announcing a new service) automatically add the related Address record(s)
	for (rr = m->ResourceRecords; rr; rr=rr->next)
		if (rr->ImmedAnswer && rr->resrec.rrtype == kDNSType_SRV)
			for (r2=m->ResourceRecords; r2; r2=r2->next)				// Scan list of resource records
				if (RRIsAddressType(r2) &&								// For all address records (A/AAAA) ...
				    ResourceRecordIsValidAnswer(r2) &&					// ... which are valid for answer ...
				    rr->LastMCTime - r2->LastMCTime >= 0 &&				// ... which we have not sent recently ...
				    rr->resrec.rdnamehash == r2->resrec.namehash &&		// ... whose name is the name of the SRV target
				    SameDomainName(&rr->resrec.rdata->u.srv.target, &r2->resrec.name) &&
				    (rr->ImmedAnswer == mDNSInterfaceMark || rr->ImmedAnswer == r2->resrec.InterfaceID))
					r2->ImmedAnswer = mDNSInterfaceMark;				// ... then mark this address record for sending too

	// If there's a record which is supposed to be unique that we're going to send, then make sure that we give
	// the whole RRSet as an atomic unit. That means that if we have any other records with the same name/type/class
	// then we need to mark them for sending too. Otherwise, if we set the kDNSClass_UniqueRRSet bit on a
	// record, then other RRSet members that have not been sent recently will get flushed out of client caches.
	// -- If a record is marked to be sent on a certain interface, make sure the whole set is marked to be sent on that interface
	// -- If any record is marked to be sent on all interfaces, make sure the whole set is marked to be sent on all interfaces
	for (rr = m->ResourceRecords; rr; rr=rr->next)
		if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
		{
			if (rr->ImmedAnswer)			// If we're sending this as answer, see that its whole RRSet is similarly marked
			{
				for (r2 = m->ResourceRecords; r2; r2=r2->next)
					if (ResourceRecordIsValidAnswer(r2))
						if (r2->ImmedAnswer != mDNSInterfaceMark && r2->ImmedAnswer != rr->ImmedAnswer && SameResourceRecordSignature(&r2->resrec, &rr->resrec))
							r2->ImmedAnswer = rr->ImmedAnswer;
			}
			else if (rr->ImmedAdditional)	// If we're sending this as additional, see that its whole RRSet is similarly marked
			{
				for (r2 = m->ResourceRecords; r2; r2=r2->next)
					if (ResourceRecordIsValidAnswer(r2))
						if (r2->ImmedAdditional != rr->ImmedAdditional && SameResourceRecordSignature(&r2->resrec, &rr->resrec))
							r2->ImmedAdditional = rr->ImmedAdditional;
			}
		}

	// Now set SendRNow state appropriately
	for (rr = m->ResourceRecords; rr; rr=rr->next)
	{
		if (rr->ImmedAnswer == mDNSInterfaceMark)		// Sending this record on all appropriate interfaces
		{
			rr->SendRNow = !intf ? mDNSNULL : (rr->resrec.InterfaceID) ? rr->resrec.InterfaceID : intf->InterfaceID;
			rr->ImmedAdditional = mDNSNULL;				// No need to send as additional if sending as answer
			rr->LastMCTime      = m->timenow;
			rr->LastMCInterface = rr->ImmedAnswer;
			// If we're announcing this record, and it's at least half-way to its ordained time, then consider this announcement done
			if (TimeToAnnounceThisRecord(rr, m->timenow + rr->ThisAPInterval/2))
			{
				rr->AnnounceCount--;
				rr->ThisAPInterval *= 2;
				rr->LastAPTime = m->timenow;
				if (rr->LastAPTime + rr->ThisAPInterval - rr->AnnounceUntil >= 0) rr->AnnounceCount = 0;
				debugf("Announcing %##s (%s) %d", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), rr->AnnounceCount);
			}
		}
		else if (rr->ImmedAnswer)						// Else, just respond to a single query on single interface:
		{
			rr->SendRNow = rr->ImmedAnswer;				// Just respond on that interface
			rr->ImmedAdditional = mDNSNULL;				// No need to send as additional too
			rr->LastMCTime      = m->timenow;
			rr->LastMCInterface = rr->ImmedAnswer;
		}
		SetNextAnnounceProbeTime(m, rr);
	}

	// ***
	// *** 2. Loop through interface list, sending records as appropriate
	// ***

	while (intf)
	{
		int numDereg    = 0;
		int numAnnounce = 0;
		int numAnswer   = 0;
		DNSMessage response;
		mDNSu8 *responseptr = response.data;
		mDNSu8 *newptr;
		InitializeDNSMessage(&response.h, zeroID, ResponseFlags);
	
		// First Pass. Look for:
		// 1. Deregistering records that need to send their goodbye packet
		// 2. Updated records that need to retract their old data
		// 3. Answers and announcements we need to send
		// In all cases, if we fail, and we've put at least one answer, we break out of the for loop so we can
		// send this packet and then try again.
		// If we have not put even one answer, then we don't bail out. We pretend we succeeded anyway,
		// because otherwise we'll end up in an infinite loop trying to send a record that will never fit.
		for (rr = m->ResourceRecords; rr; rr=rr->next)
			if (rr->SendRNow == intf->InterfaceID)
			{
				if (rr->resrec.RecordType == kDNSRecordTypeDeregistering)
				{
					newptr = PutResourceRecordTTL(&response, responseptr, &response.h.numAnswers, &rr->resrec, 0);
					if (!newptr && response.h.numAnswers) break;
					numDereg++;
					responseptr = newptr;
				}
				else if (rr->NewRData)							// If we have new data for this record
				{
					RData *OldRData     = rr->resrec.rdata;
					mDNSu16 oldrdlength = rr->resrec.rdlength;
					// See if we should send a courtesy "goodbye" the old data before we replace it.
					// We compare with "InitialAnnounceCount-1" instead of "InitialAnnounceCount" because by the time
					// we get to this place in this routine we've we've already decremented rr->AnnounceCount
					if (ResourceRecordIsValidAnswer(rr) && rr->AnnounceCount < InitialAnnounceCount-1)
					{
						newptr = PutResourceRecordTTL(&response, responseptr, &response.h.numAnswers, &rr->resrec, 0);
						if (!newptr && response.h.numAnswers) break;
						numDereg++;
						responseptr = newptr;
					}
					// Now try to see if we can fit the update in the same packet (not fatal if we can't)
					SetNewRData(&rr->resrec, rr->NewRData, rr->newrdlength);
					if ((rr->resrec.RecordType & kDNSRecordTypeUniqueMask) && HaveSentEntireRRSet(m, rr, intf->InterfaceID))
						rr->resrec.rrclass |= kDNSClass_UniqueRRSet;		// Temporarily set the cache flush bit so PutResourceRecord will set it
					newptr = PutResourceRecord(&response, responseptr, &response.h.numAnswers, &rr->resrec);
					rr->resrec.rrclass &= ~kDNSClass_UniqueRRSet;			// Make sure to clear cache flush bit back to normal state
					if (newptr) responseptr = newptr;
					SetNewRData(&rr->resrec, OldRData, oldrdlength);
				}
				else
				{
					if ((rr->resrec.RecordType & kDNSRecordTypeUniqueMask) && HaveSentEntireRRSet(m, rr, intf->InterfaceID))
						rr->resrec.rrclass |= kDNSClass_UniqueRRSet;		// Temporarily set the cache flush bit so PutResourceRecord will set it
					newptr = PutResourceRecordTTL(&response, responseptr, &response.h.numAnswers, &rr->resrec, m->SleepState ? 0 : rr->resrec.rroriginalttl);
					rr->resrec.rrclass &= ~kDNSClass_UniqueRRSet;			// Make sure to clear cache flush bit back to normal state
					if (!newptr && response.h.numAnswers) break;
					if (rr->LastAPTime == m->timenow) numAnnounce++; else numAnswer++;
					responseptr = newptr;
				}
				// If sending on all interfaces, go to next interface; else we're finished now
				if (rr->ImmedAnswer == mDNSInterfaceMark && rr->resrec.InterfaceID == mDNSInterface_Any)
					rr->SendRNow = GetNextActiveInterfaceID(intf);
				else
					rr->SendRNow = mDNSNULL;
			}
	
		// Second Pass. Add additional records, if there's space.
		newptr = responseptr;
		for (rr = m->ResourceRecords; rr; rr=rr->next)
			if (rr->ImmedAdditional == intf->InterfaceID)
			{
				// Since additionals are optional, we clear ImmedAdditional anyway, even if we subsequently find it doesn't fit in the packet
				rr->ImmedAdditional = mDNSNULL;
				if (newptr && ResourceRecordIsValidAnswer(rr))
				{
					if (rr->resrec.RecordType & kDNSRecordTypeUniqueMask)
					{
						// Try to find another member of this set that we're still planning to send on this interface
						const AuthRecord *a;
						for (a = m->ResourceRecords; a; a=a->next)
							if (a->ImmedAdditional == intf->InterfaceID && SameResourceRecordSignature(&a->resrec, &rr->resrec)) break;
						if (a == mDNSNULL)							// If no more members of this set found
							rr->resrec.rrclass |= kDNSClass_UniqueRRSet;	// Temporarily set the cache flush bit so PutResourceRecord will set it
					}
					newptr = PutResourceRecord(&response, newptr, &response.h.numAdditionals, &rr->resrec);
					if (newptr) responseptr = newptr;
					rr->resrec.rrclass &= ~kDNSClass_UniqueRRSet;			// Make sure to clear cache flush bit back to normal state
				}
			}
	
		if (response.h.numAnswers > 0)	// We *never* send a packet with only additionals in it
		{
			debugf("SendResponses: Sending %d Deregistration%s, %d Announcement%s, %d Answer%s, %d Additional%s on %p",
			       numDereg,                  numDereg                  == 1 ? "" : "s",
			       numAnnounce,               numAnnounce               == 1 ? "" : "s",
			       numAnswer,                 numAnswer                 == 1 ? "" : "s",
			       response.h.numAdditionals, response.h.numAdditionals == 1 ? "" : "s", intf->InterfaceID);
			mDNSSendDNSMessage(m, &response, responseptr, intf->InterfaceID, MulticastDNSPort, &AllDNSLinkGroup_v4, MulticastDNSPort);
			mDNSSendDNSMessage(m, &response, responseptr, intf->InterfaceID, MulticastDNSPort, &AllDNSLinkGroup_v6, MulticastDNSPort);
			if (!m->SuppressSending) m->SuppressSending = (m->timenow + mDNSPlatformOneSecond/10) | 1;	// OR with one to ensure non-zero
			if (++pktcount >= 1000)
			{ LogMsg("SendResponses exceeded loop limit %d: giving up", pktcount); break; }
			// There might be more things to send on this interface, so go around one more time and try again.
		}
		else	// Nothing more to send on this interface; go to next
		{
			const NetworkInterfaceInfo *next = GetFirstActiveInterface(intf->next);
#if MDNS_DEBUGMSGS && 0
			const char *const msg = next ? "SendResponses: Nothing more on %p; moving to %p" : "SendResponses: Nothing more on %p";
			debugf(msg, intf, next);
#endif
			intf = next;
		}
	}

	// ***
	// *** 3. Cleanup: Now that everything is sent, call client callback functions, and reset state variables
	// ***

	if (m->CurrentRecord) LogMsg("SendResponses: ERROR m->CurrentRecord already set");
	m->CurrentRecord = m->ResourceRecords;
	while (m->CurrentRecord)
	{
		rr = m->CurrentRecord;
		m->CurrentRecord = rr->next;

		if (rr->NewRData)
		{
			RData *OldRData = rr->resrec.rdata;
			SetNewRData(&rr->resrec, rr->NewRData, rr->newrdlength);	// Update our rdata
			rr->NewRData = mDNSNULL;									// Clear the NewRData pointer ...
			if (rr->UpdateCallback)
				rr->UpdateCallback(m, rr, OldRData);					// ... and let the client know
		}

		if (rr->resrec.RecordType == kDNSRecordTypeDeregistering)
			CompleteDeregistration(m, rr);
		else
		{
			rr->ImmedAnswer = mDNSNULL;
			rr->v4Requester = zeroIPAddr;
			rr->v6Requester = zerov6Addr;
		}
	}
	verbosedebugf("SendResponses: Next in %d ticks", m->NextScheduledResponse - m->timenow);
}

// Calling CheckCacheExpiration() is an expensive operation because it has to look at the entire cache,
// so we want to be lazy about how frequently we do it.
// 1. If a cache record is currently referenced by *no* active questions,
//    then we don't mind expiring it up to a minute late (who will know?)
// 2. Else, if a cache record is due for some of its final expiration queries,
//    we'll allow them to be late by up to 2% of the TTL
// 3. Else, if a cache record has completed all its final expiration queries without success,
//    and is expiring, and had an original TTL more than ten seconds, we'll allow it to be one second late
// 4. Else, it is expiring and had an original TTL of ten seconds or less (includes explicit goodbye packets),
//    so allow at most 1/10 second lateness
#define CacheCheckGracePeriod(RR) (                                                   \
	((RR)->CRActiveQuestion == mDNSNULL            ) ? (60 * mDNSPlatformOneSecond) : \
	((RR)->UnansweredQueries < MaxUnansweredQueries) ? (TicksTTL(rr)/50)            : \
	((RR)->resrec.rroriginalttl > 10               ) ? (mDNSPlatformOneSecond)      : (mDNSPlatformOneSecond/10))

// Note: MUST call SetNextCacheCheckTime any time we change:
// rr->TimeRcvd
// rr->resrec.rroriginalttl
// rr->UnansweredQueries
// rr->CRActiveQuestion
mDNSlocal void SetNextCacheCheckTime(mDNS *const m, CacheRecord *const rr)
{
	rr->NextRequiredQuery = RRExpireTime(rr);

	// If we have an active question, then see if we want to schedule a refresher query for this record.
	// Usually we expect to do four queries, at 80-82%, 85-87%, 90-92% and then 95-97% of the TTL.
	if (rr->CRActiveQuestion && rr->UnansweredQueries < MaxUnansweredQueries)
	{
		rr->NextRequiredQuery -= TicksTTL(rr)/20 * (MaxUnansweredQueries - rr->UnansweredQueries);
		rr->NextRequiredQuery += mDNSRandom((mDNSu32)TicksTTL(rr)/50);
		verbosedebugf("SetNextCacheCheckTime: %##s (%s) NextRequiredQuery in %ld sec",
			      rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), (rr->NextRequiredQuery - m->timenow) / mDNSPlatformOneSecond);
	}

	if (m->NextCacheCheck - (rr->NextRequiredQuery + CacheCheckGracePeriod(rr)) > 0)
		m->NextCacheCheck = (rr->NextRequiredQuery + CacheCheckGracePeriod(rr));
}

#define kDefaultReconfirmTimeForNoAnswer        ((mDNSu32)mDNSPlatformOneSecond * 45)
#define kDefaultReconfirmTimeForCableDisconnect ((mDNSu32)mDNSPlatformOneSecond *  5)
#define kMinimumReconfirmTime                   ((mDNSu32)mDNSPlatformOneSecond *  5)

mDNSlocal mStatus mDNS_Reconfirm_internal(mDNS *const m, CacheRecord *const rr, mDNSu32 interval)
{
	if (interval < kMinimumReconfirmTime)
		interval = kMinimumReconfirmTime;
	if (interval > 0x10000000)	// Make sure interval doesn't overflow when we multiply by four below
		interval = 0x10000000;

	// If the expected expiration time for this record is more than interval+33%, then accelerate its expiration
	if (RRExpireTime(rr) - m->timenow > (mDNSs32)((interval * 4) / 3))
	{
		// Add a 33% random amount to the interval, to avoid synchronization between multiple hosts
		interval += mDNSRandom(interval/3);
		rr->TimeRcvd          = m->timenow - (mDNSs32)interval * 3;
		rr->resrec.rroriginalttl     = interval * 4 / mDNSPlatformOneSecond;
		SetNextCacheCheckTime(m, rr);
	}
	debugf("mDNS_Reconfirm_internal:%5ld ticks to go for %s", RRExpireTime(rr) - m->timenow, GetRRDisplayString(m, rr));
	return(mStatus_NoError);
}

#define MaxQuestionInterval         (3600 * mDNSPlatformOneSecond)

// BuildQuestion puts a question into a DNS Query packet and if successful, updates the value of queryptr.
// It also appends to the list of known answer records that need to be included,
// and updates the forcast for the size of the known answer section.
mDNSlocal mDNSBool BuildQuestion(mDNS *const m, DNSMessage *query, mDNSu8 **queryptr, DNSQuestion *q,
				 CacheRecord ***kalistptrptr, mDNSu32 *answerforecast)
{
	mDNSBool ucast = q->LargeAnswers || q->ThisQInterval <= InitialQuestionInterval*2;
	mDNSu16 ucbit = (mDNSu16)(ucast ? kDNSQClass_UnicastResponse : 0);
	const mDNSu8 *const limit = query->data + NormalMaxDNSMessageData;
	mDNSu8 *newptr = putQuestion(query, *queryptr, limit, &q->qname, q->qtype, (mDNSu16)(q->qclass | ucbit));
	if (!newptr)
	{
		debugf("BuildQuestion: No more space in this packet for question %##s", q->qname.c);
		return(mDNSfalse);
	}
	else if (newptr + *answerforecast >= limit)
	{
		verbosedebugf("BuildQuestion: Retracting question %##s new forecast total %d", q->qname.c, newptr + *answerforecast - query->data);
		query->h.numQuestions--;
		return(mDNSfalse);
	}
	else
	{
		mDNSu32 forecast = *answerforecast;
		CacheRecord *rr;
		CacheRecord **ka = *kalistptrptr;	// Make a working copy of the pointer we're going to update

		for (rr=m->rrcache_hash[HashSlot(&q->qname)]; rr; rr=rr->next)		// If we have a resource record in our cache,
			if (rr->resrec.InterfaceID == q->SendQNow &&					// received on this interface
			    rr->NextInKAList == mDNSNULL && ka != &rr->NextInKAList &&	// which is not already in the known answer list
			    rr->resrec.rdlength <= SmallRecordLimit &&					// which is small enough to sensibly fit in the packet
			    ResourceRecordAnswersQuestion(&rr->resrec, q) &&			// which answers our question
			    rr->TimeRcvd + TicksTTL(rr)/2 - m->timenow >= 0 &&			// and it is less than half-way to expiry
			    rr->NextRequiredQuery - (m->timenow + q->ThisQInterval) > 0)// and we'll ask at least once again before NextRequiredQuery
			{
				*ka = rr;	// Link this record into our known answer chain
				ka = &rr->NextInKAList;
				// We forecast: compressed name (2) type (2) class (2) TTL (4) rdlength (2) rdata (n)
				forecast += 12 + rr->resrec.rdestimate;
				// If we're trying to put more than one question in this packet, and it doesn't fit
				// then undo that last question and try again next time
				if (query->h.numQuestions > 1 && newptr + forecast >= limit)
				{
					debugf("BuildQuestion: Retracting question %##s (%s) new forecast total %d",
					       q->qname.c, DNSTypeName(q->qtype), newptr + forecast - query->data);
					query->h.numQuestions--;
					ka = *kalistptrptr;		// Go back to where we started and retract these answer records
					while (*ka) { CacheRecord *rr = *ka; *ka = mDNSNULL; ka = &rr->NextInKAList; }
					return(mDNSfalse);		// Return false, so we'll try again in the next packet
				}
			}

		// Traffic reduction:
		// If we already have at least one unique answer in the cache,
		// OR we have so many shared answers that the KA list is too big to fit in one packet
		// The we suppress queries number 3 and 5:
		// Query 1 (immediately;      ThisQInterval =  1 sec; request unicast replies)
		// Query 2 (after  1 second;  ThisQInterval =  2 sec; send normally)
		// Query 3 (after  2 seconds; ThisQInterval =  4 sec; may suppress)
		// Query 4 (after  4 seconds; ThisQInterval =  8 sec; send normally)
		// Query 5 (after  8 seconds; ThisQInterval = 16 sec; may suppress)
		// Query 6 (after 16 seconds; ThisQInterval = 32 sec; send normally)
		if (q->UniqueAnswers || newptr + forecast >= limit)
			if (q->ThisQInterval == InitialQuestionInterval * 8 || q->ThisQInterval == InitialQuestionInterval * 32)
			{
				query->h.numQuestions--;
				ka = *kalistptrptr;		// Go back to where we started and retract these answer records
				while (*ka) { CacheRecord *rr = *ka; *ka = mDNSNULL; ka = &rr->NextInKAList; }
				return(mDNStrue);		// Return true: pretend we succeeded, even though we actually suppressed this question
			}

		// Success! Update our state pointers, increment UnansweredQueries as appropriate, and return
		*queryptr        = newptr;				// Update the packet pointer
		*answerforecast  = forecast;			// Update the forecast
		*kalistptrptr    = ka;					// Update the known answer list pointer
		if (ucast) m->ExpectUnicastResponse = m->timenow;

		for (rr=m->rrcache_hash[HashSlot(&q->qname)]; rr; rr=rr->next)		// For every resource record in our cache,
			if (rr->resrec.InterfaceID == q->SendQNow &&					// received on this interface
			    rr->NextInKAList == mDNSNULL && ka != &rr->NextInKAList &&	// which is not in the known answer list
			    ResourceRecordAnswersQuestion(&rr->resrec, q))				// which answers our question
			{
				rr->UnansweredQueries++;								// indicate that we're expecting a response
				rr->LastUnansweredTime = m->timenow;
				SetNextCacheCheckTime(m, rr);
			}

		return(mDNStrue);
	}
}

mDNSlocal void ReconfirmAntecedents(mDNS *const m, DNSQuestion *q)
{
	mDNSu32 slot;
	CacheRecord *rr;
	domainname *target = NULL;
	for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
		for (rr = m->rrcache_hash[slot]; rr; rr=rr->next)
		{
			mDNSBool b = SameDomainName(target, &q->qname);
			target = GetRRDomainNameTarget(&rr->resrec);
			if ((target) && rr->resrec.rdnamehash == q->qnamehash && b)
			{
				mDNS_Reconfirm_internal(m, rr, kDefaultReconfirmTimeForNoAnswer);
			}
		}
}

// Only DupSuppressInfos newer than the specified 'time' are allowed to remain active
mDNSlocal void ExpireDupSuppressInfo(DupSuppressInfo ds[DupSuppressInfoSize], mDNSs32 time)
{
	int i;
	for (i=0; i<DupSuppressInfoSize; i++) if (ds[i].Time - time < 0) ds[i].InterfaceID = mDNSNULL;
}

mDNSlocal void ExpireDupSuppressInfoOnInterface(DupSuppressInfo ds[DupSuppressInfoSize], mDNSs32 time, mDNSInterfaceID InterfaceID)
{
	int i;
	for (i=0; i<DupSuppressInfoSize; i++) if (ds[i].InterfaceID == InterfaceID && ds[i].Time - time < 0) ds[i].InterfaceID = mDNSNULL;
}

mDNSlocal mDNSBool SuppressOnThisInterface(const DupSuppressInfo ds[DupSuppressInfoSize], const NetworkInterfaceInfo * const intf)
{
	int i;
	mDNSBool v4 = !intf->IPv4Available;		// If this interface doesn't do v4, we don't need to find a v4 duplicate of this query
	mDNSBool v6 = !intf->IPv6Available;		// If this interface doesn't do v6, we don't need to find a v6 duplicate of this query
	for (i=0; i<DupSuppressInfoSize; i++)
		if (ds[i].InterfaceID == intf->InterfaceID)
		{
			if      (ds[i].Type == mDNSAddrType_IPv4) v4 = mDNStrue;
			else if (ds[i].Type == mDNSAddrType_IPv6) v6 = mDNStrue;
			if (v4 && v6) return(mDNStrue);
		}
	return(mDNSfalse);
}

mDNSlocal int RecordDupSuppressInfo(DupSuppressInfo ds[DupSuppressInfoSize], mDNSs32 Time, mDNSInterfaceID InterfaceID, mDNSs32 Type)
{
	int i, j;

	// See if we have this one in our list somewhere already
	for (i=0; i<DupSuppressInfoSize; i++) if (ds[i].InterfaceID == InterfaceID && ds[i].Type == Type) break;

	// If not, find a slot we can re-use
	if (i >= DupSuppressInfoSize)
	{
		i = 0;
		for (j=1; j<DupSuppressInfoSize && ds[i].InterfaceID; j++)
			if (!ds[j].InterfaceID || ds[j].Time - ds[i].Time < 0)
				i = j;
	}
	
	// Record the info about this query we saw
	ds[i].Time        = Time;
	ds[i].InterfaceID = InterfaceID;
	ds[i].Type        = Type;
	
	return(i);
}

mDNSlocal mDNSBool AccelerateThisQuery(mDNS *const m, DNSQuestion *q)
{
	// If more than 90% of the way to the query time, we should unconditionally accelerate it
	if (TimeToSendThisQuestion(q, m->timenow + q->ThisQInterval/10))
		return(mDNStrue);

	// If half-way to next scheduled query time, only accelerate if it will add less than 512 bytes to the packet
	if (TimeToSendThisQuestion(q, m->timenow + q->ThisQInterval/2))
	{
		// We forecast: qname (n) type (2) class (2)
		mDNSu32 forecast = (mDNSu32)DomainNameLength(&q->qname) + 4;
		CacheRecord *rr;
		for (rr=m->rrcache_hash[HashSlot(&q->qname)]; rr; rr=rr->next)		// If we have a resource record in our cache,
			if (rr->resrec.rdlength <= SmallRecordLimit &&					// which is small enough to sensibly fit in the packet
			    ResourceRecordAnswersQuestion(&rr->resrec, q) &&			// which answers our question
			    rr->TimeRcvd + TicksTTL(rr)/2 - m->timenow >= 0 &&			// and it is less than half-way to expiry
			    rr->NextRequiredQuery - (m->timenow + q->ThisQInterval) > 0)// and we'll ask at least once again before NextRequiredQuery
			{
				// We forecast: compressed name (2) type (2) class (2) TTL (4) rdlength (2) rdata (n)
				forecast += 12 + rr->resrec.rdestimate;
				if (forecast >= 512) return(mDNSfalse);	// If this would add 512 bytes or more to the packet, don't accelerate
			}
		return(mDNStrue);
	}

	return(mDNSfalse);
}

// How Standard Queries are generated:
// 1. The Question Section contains the question
// 2. The Additional Section contains answers we already know, to suppress duplicate responses

// How Probe Queries are generated:
// 1. The Question Section contains queries for the name we intend to use, with QType=ANY because
// if some other host is already using *any* records with this name, we want to know about it.
// 2. The Authority Section contains the proposed values we intend to use for one or more
// of our records with that name (analogous to the Update section of DNS Update packets)
// because if some other host is probing at the same time, we each want to know what the other is
// planning, in order to apply the tie-breaking rule to see who gets to use the name and who doesn't.

mDNSlocal void SendQueries(mDNS *const m)
{
	int pktcount = 0;
	DNSQuestion *q;
	// For explanation of maxExistingQuestionInterval logic, see comments for maxExistingAnnounceInterval
	mDNSs32 maxExistingQuestionInterval = 0;
	const NetworkInterfaceInfo *intf = GetFirstActiveInterface(m->HostInterfaces);
	CacheRecord *KnownAnswerList = mDNSNULL;

	// 1. If time for a query, work out what we need to do
	if (m->timenow - m->NextScheduledQuery >= 0)
	{
		mDNSu32 slot;
		CacheRecord *rr;
		m->NextScheduledQuery = m->timenow + kNextScheduledTime;

		// We're expecting to send a query anyway, so see if any expiring cache records are close enough
		// to their NextRequiredQuery to be worth batching them together with this one
		for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
			for (rr = m->rrcache_hash[slot]; rr; rr=rr->next)
				if (rr->CRActiveQuestion && rr->UnansweredQueries < MaxUnansweredQueries)
					if (m->timenow + TicksTTL(rr)/50 - rr->NextRequiredQuery >= 0)
					{
						q = rr->CRActiveQuestion;
						ExpireDupSuppressInfoOnInterface(q->DupSuppress, m->timenow - TicksTTL(rr)/20, rr->resrec.InterfaceID);
						if      (q->SendQNow == mDNSNULL)               q->SendQNow = rr->resrec.InterfaceID;
						else if (q->SendQNow != rr->resrec.InterfaceID) q->SendQNow = mDNSInterfaceMark;
					}

		// Scan our list of questions to see which ones we're definitely going to send
		for (q = m->Questions; q; q=q->next)
			if (TimeToSendThisQuestion(q, m->timenow))
			{
				q->SendQNow = mDNSInterfaceMark;		// Mark this question for sending on all interfaces
				if (maxExistingQuestionInterval < q->ThisQInterval)
					maxExistingQuestionInterval = q->ThisQInterval;
			}
	
		// Scan our list of questions
		// (a) to see if there are any more that are worth accelerating, and
		// (b) to update the state variables for all the questions we're going to send
		for (q = m->Questions; q; q=q->next)
		{
			if (q->SendQNow || (ActiveQuestion(q) && q->ThisQInterval <= maxExistingQuestionInterval && AccelerateThisQuery(m,q)))
			{
				// If at least halfway to next query time, advance to next interval
				// If less than halfway to next query time, treat this as logically a repeat of the last transmission, without advancing the interval
				if (m->timenow - (q->LastQTime + q->ThisQInterval/2) >= 0)
				{
					q->SendQNow = mDNSInterfaceMark;	// Mark this question for sending on all interfaces
					q->ThisQInterval *= 2;
					if (q->ThisQInterval > MaxQuestionInterval)
						q->ThisQInterval = MaxQuestionInterval;
					else if (q->CurrentAnswers == 0 && q->ThisQInterval == InitialQuestionInterval * 8)
					{
						debugf("SendQueries: Zero current answers for %##s (%s); will reconfirm antecedents", q->qname.c, DNSTypeName(q->qtype));
						ReconfirmAntecedents(m, q);		// If sending third query, and no answers yet, time to begin doubting the source
					}
				}

				// Mark for sending. (If no active interfaces, then don't even try.)
				q->SendOnAll = (q->SendQNow == mDNSInterfaceMark);
				if (q->SendOnAll)
				{
					q->SendQNow  = !intf ? mDNSNULL : (q->InterfaceID) ? q->InterfaceID : intf->InterfaceID;
					q->LastQTime = m->timenow;
				}

				// If we recorded a duplicate suppression for this question less than half an interval ago,
				// then we consider it recent enough that we don't need to do an identical query ourselves.
				ExpireDupSuppressInfo(q->DupSuppress, m->timenow - q->ThisQInterval/2);

				q->LastQTxTime   = m->timenow;
				q->RecentAnswers = 0;
			}
			// For all questions (not just the ones we're sending) check what the next scheduled event will be
			SetNextQueryTime(m,q);
		}
	}

	// 2. Scan our authoritative RR list to see what probes we might need to send
	if (m->timenow - m->NextScheduledProbe >= 0)
	{
		m->NextScheduledProbe = m->timenow + kNextScheduledTime;

		if (m->CurrentRecord) LogMsg("SendQueries:   ERROR m->CurrentRecord already set");
		m->CurrentRecord = m->ResourceRecords;
		while (m->CurrentRecord)
		{
			AuthRecord *rr = m->CurrentRecord;
			m->CurrentRecord = rr->next;
			if (rr->resrec.RecordType == kDNSRecordTypeUnique)			// For all records that are still probing...
			{
				// 1. If it's not reached its probe time, just make sure we update m->NextScheduledProbe correctly
				if (m->timenow - (rr->LastAPTime + rr->ThisAPInterval) < 0)
				{
					SetNextAnnounceProbeTime(m, rr);
				}
				// 2. else, if it has reached its probe time, mark it for sending and then update m->NextScheduledProbe correctly
				else if (rr->ProbeCount)
				{
					// Mark for sending. (If no active interfaces, then don't even try.)
					rr->SendRNow   = !intf ? mDNSNULL : (rr->resrec.InterfaceID) ? rr->resrec.InterfaceID : intf->InterfaceID;
					rr->LastAPTime = m->timenow;
					rr->ProbeCount--;
					SetNextAnnounceProbeTime(m, rr);
				}
				// else, if it has now finished probing, move it to state Verified, and update m->NextScheduledResponse so it will be announced
				else
				{
					AuthRecord *r2;
					rr->resrec.RecordType     = kDNSRecordTypeVerified;
					rr->ThisAPInterval = DefaultAnnounceIntervalForTypeUnique;
					rr->LastAPTime     = m->timenow - DefaultAnnounceIntervalForTypeUnique;
					SetNextAnnounceProbeTime(m, rr);
					// If we have any records on our duplicate list that match this one, they have now also completed probing
					for (r2 = m->DuplicateRecords; r2; r2=r2->next)
						if (r2->resrec.RecordType == kDNSRecordTypeUnique && RecordIsLocalDuplicate(r2, rr))
							r2->ProbeCount = 0;
					CompleteProbing(m, rr);
				}
			}
		}
		m->CurrentRecord = m->DuplicateRecords;
		while (m->CurrentRecord)
		{
			AuthRecord *rr = m->CurrentRecord;
			m->CurrentRecord = rr->next;
			if (rr->resrec.RecordType == kDNSRecordTypeUnique && rr->ProbeCount == 0)
				CompleteProbing(m, rr);
		}
	}

	// 3. Now we know which queries and probes we're sending, go through our interface list sending the appropriate queries on each interface
	while (intf)
	{
		AuthRecord *rr;
		DNSMessage query;
		mDNSu8 *queryptr = query.data;
		InitializeDNSMessage(&query.h, zeroID, QueryFlags);
		if (KnownAnswerList) verbosedebugf("SendQueries:   KnownAnswerList set... Will continue from previous packet");
		if (!KnownAnswerList)
		{
			// Start a new known-answer list
			CacheRecord **kalistptr = &KnownAnswerList;
			mDNSu32 answerforecast = 0;
			
			// Put query questions in this packet
			for (q = m->Questions; q; q=q->next)
				if (q->SendQNow == intf->InterfaceID)
				{
					debugf("SendQueries: %s question for %##s (%s) at %lu forecast total %lu",
					       SuppressOnThisInterface(q->DupSuppress, intf) ? "Suppressing" : "Putting    ",
					       q->qname.c, DNSTypeName(q->qtype), queryptr - query.data, queryptr + answerforecast - query.data);
					// If we're suppressing this question, or we successfully put it, update its SendQNow state
					if (SuppressOnThisInterface(q->DupSuppress, intf) ||
					    BuildQuestion(m, &query, &queryptr, q, &kalistptr, &answerforecast))
						q->SendQNow = (q->InterfaceID || !q->SendOnAll) ? mDNSNULL : GetNextActiveInterfaceID(intf);
				}

			// Put probe questions in this packet
			for (rr = m->ResourceRecords; rr; rr=rr->next)
				if (rr->SendRNow == intf->InterfaceID)
				{
					mDNSBool ucast = rr->ProbeCount >= DefaultProbeCountForTypeUnique-1;
					mDNSu16 ucbit = (mDNSu16)(ucast ? kDNSQClass_UnicastResponse : 0);
					const mDNSu8 *const limit = query.data + ((query.h.numQuestions) ? NormalMaxDNSMessageData : AbsoluteMaxDNSMessageData);
					mDNSu8 *newptr = putQuestion(&query, queryptr, limit, &rr->resrec.name, kDNSQType_ANY, (mDNSu16)(rr->resrec.rrclass | ucbit));
					// We forecast: compressed name (2) type (2) class (2) TTL (4) rdlength (2) rdata (n)
					mDNSu32 forecast = answerforecast + 12 + rr->resrec.rdestimate;
					if (newptr && newptr + forecast < limit)
					{
						queryptr       = newptr;
						answerforecast = forecast;
						rr->SendRNow = (rr->resrec.InterfaceID) ? mDNSNULL : GetNextActiveInterfaceID(intf);
						rr->IncludeInProbe = mDNStrue;
						verbosedebugf("SendQueries:   Put Question %##s (%s) probecount %d", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), rr->ProbeCount);
					}
					else
					{
						verbosedebugf("SendQueries:   Retracting Question %##s (%s)", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
						query.h.numQuestions--;
					}
				}
		}

		// Put our known answer list (either new one from this question or questions, or remainder of old one from last time)
		while (KnownAnswerList)
		{
			CacheRecord *rr = KnownAnswerList;
			mDNSu32 SecsSinceRcvd = ((mDNSu32)(m->timenow - rr->TimeRcvd)) / mDNSPlatformOneSecond;
			mDNSu8 *newptr = PutResourceRecordTTL(&query, queryptr, &query.h.numAnswers, &rr->resrec, rr->resrec.rroriginalttl - SecsSinceRcvd);
			if (newptr)
			{
				verbosedebugf("SendQueries:   Put %##s (%s) at %lu - %lu", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), queryptr - query.data, newptr - query.data);
				queryptr = newptr;
				KnownAnswerList = rr->NextInKAList;
				rr->NextInKAList = mDNSNULL;
			}
			else
			{
				// If we ran out of space and we have more than one question in the packet, that's an error --
				// we shouldn't have put more than one question if there was a risk of us running out of space.
				if (query.h.numQuestions > 1)
					LogMsg("SendQueries:   Put %d answers; No more space for known answers", query.h.numAnswers);
				query.h.flags.b[0] |= kDNSFlag0_TC;
				break;
			}
		}

		for (rr = m->ResourceRecords; rr; rr=rr->next)
			if (rr->IncludeInProbe)
			{
				mDNSu8 *newptr = PutResourceRecord(&query, queryptr, &query.h.numAuthorities, &rr->resrec);
				rr->IncludeInProbe = mDNSfalse;
				if (newptr) queryptr = newptr;
				else LogMsg("SendQueries:   How did we fail to have space for the Update record %##s (%s)?",
					    rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
			}
		
		if (queryptr > query.data)
		{
			if ((query.h.flags.b[0] & kDNSFlag0_TC) && query.h.numQuestions > 1)
				LogMsg("SendQueries: Should not have more than one question (%d) in a truncated packet\n", query.h.numQuestions);
			debugf("SendQueries:   Sending %d Question%s %d Answer%s %d Update%s on %p",
			       query.h.numQuestions,   query.h.numQuestions   == 1 ? "" : "s",
			       query.h.numAnswers,     query.h.numAnswers     == 1 ? "" : "s",
			       query.h.numAuthorities, query.h.numAuthorities == 1 ? "" : "s", intf->InterfaceID);
			mDNSSendDNSMessage(m, &query, queryptr, intf->InterfaceID, MulticastDNSPort, &AllDNSLinkGroup_v4, MulticastDNSPort);
			mDNSSendDNSMessage(m, &query, queryptr, intf->InterfaceID, MulticastDNSPort, &AllDNSLinkGroup_v6, MulticastDNSPort);
			if (!m->SuppressSending) m->SuppressSending = (m->timenow + mDNSPlatformOneSecond/10) | 1;	// OR with one to ensure non-zero
			if (++pktcount >= 1000)
			{ LogMsg("SendQueries exceeded loop limit %d: giving up", pktcount); break; }
			// There might be more records left in the known answer list, or more questions to send
			// on this interface, so go around one more time and try again.
		}
		else	// Nothing more to send on this interface; go to next
		{
			const NetworkInterfaceInfo *next = GetFirstActiveInterface(intf->next);
#if MDNS_DEBUGMSGS && 0
			const char *const msg = next ? "SendQueries:   Nothing more on %p; moving to %p" : "SendQueries:   Nothing more on %p";
			debugf(msg, intf, next);
#endif
			intf = next;
		}
	}
}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - RR List Management & Task Management
#endif

// NOTE: AnswerQuestionWithResourceRecord can call a user callback, which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void AnswerQuestionWithResourceRecord(mDNS *const m, DNSQuestion *q, CacheRecord *rr, mDNSBool AddRecord)
{
	verbosedebugf("AnswerQuestionWithResourceRecord:%4lu %s TTL%6lu %##s (%s)",
		      q->CurrentAnswers, AddRecord ? "Add" : "Rmv", rr->resrec.rroriginalttl, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));

	rr->LastUsed = m->timenow;
	rr->UseCount++;
	if (ActiveQuestion(q) && rr->CRActiveQuestion != q)
	{
		if (!rr->CRActiveQuestion) m->rrcache_active++;	// If not previously active, increment rrcache_active count
		rr->CRActiveQuestion = q;						// We know q is non-null
		SetNextCacheCheckTime(m, rr);
	}

	// CAUTION: MUST NOT do anything more with q after calling q->Callback(), because the client's callback function
	// is allowed to do anything, including starting/stopping queries, registering/deregistering records, etc.
	// Right now the only routines that call AnswerQuestionWithResourceRecord() are CacheRecordAdd(), CacheRecordRmv()
	// and AnswerNewQuestion(), and all of them use the "m->CurrentQuestion" mechanism to protect against questions
	// being deleted out from under them.
	m->mDNS_reentrancy++; // Increment to allow client to legally make mDNS API calls from the callback
	if (q->QuestionCallback)
		q->QuestionCallback(m, q, &rr->resrec, AddRecord);
	m->mDNS_reentrancy--; // Decrement to block mDNS API calls again
}

// CacheRecordAdd is only called from mDNSCoreReceiveResponse, *never* directly as a result of a client API call.
// If new questions are created as a result of invoking client callbacks, they will be added to
// the end of the question list, and m->NewQuestions will be set to indicate the first new question.
// rr is a new CacheRecord just received into our cache
// (kDNSRecordTypePacketAns/PacketAnsUnique/PacketAdd/PacketAddUnique).
// NOTE: CacheRecordAdd calls AnswerQuestionWithResourceRecord which can call a user callback,
// which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void CacheRecordAdd(mDNS *const m, CacheRecord *rr)
{
	if (m->CurrentQuestion) LogMsg("CacheRecordAdd ERROR m->CurrentQuestion already set");
	m->CurrentQuestion = m->Questions;
	while (m->CurrentQuestion && m->CurrentQuestion != m->NewQuestions)
	{
		DNSQuestion *q = m->CurrentQuestion;
		m->CurrentQuestion = q->next;
		if (ResourceRecordAnswersQuestion(&rr->resrec, q))
		{
			// If this question is one that's actively sending queries, and it's received ten answers within one second of sending the last
			// query packet, then that indicates some radical network topology change, so reset its exponential backoff back to the start.
			// We must be at least at the eight-second interval to do this. If we're at the four-second interval, or less,
			// there's not much benefit accelerating because we will anyway send another query within a few seconds.
			// The first reset query is sent out randomized over the next four seconds to reduce possible synchronization between machines.
			if (ActiveQuestion(q) && ++q->RecentAnswers >= 10 &&
			    q->ThisQInterval > InitialQuestionInterval*16 && m->timenow - q->LastQTxTime < mDNSPlatformOneSecond)
			{
				LogMsg("CacheRecordAdd: %##s (%s) got immediate answer burst; restarting exponential backoff sequence",
				       q->qname.c, DNSTypeName(q->qtype));
				q->LastQTime     = m->timenow - InitialQuestionInterval + (mDNSs32)mDNSRandom((mDNSu32)mDNSPlatformOneSecond*4);
				q->ThisQInterval = InitialQuestionInterval;
				SetNextQueryTime(m,q);
			}
			verbosedebugf("CacheRecordAdd %p %##s (%s) %lu", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), rr->resrec.rroriginalttl);
			q->CurrentAnswers++;
			if (rr->resrec.rdlength > SmallRecordLimit) q->LargeAnswers++;
			if (rr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) q->UniqueAnswers++;
			AnswerQuestionWithResourceRecord(m, q, rr, mDNStrue);
			// MUST NOT dereference q again after calling AnswerQuestionWithResourceRecord()
		}
	}
	m->CurrentQuestion = mDNSNULL;
}

// CacheRecordRmv is only called from CheckCacheExpiration, which is called from mDNS_Execute
// If new questions are created as a result of invoking client callbacks, they will be added to
// the end of the question list, and m->NewQuestions will be set to indicate the first new question.
// rr is an existing cache CacheRecord that just expired and is being deleted
// (kDNSRecordTypePacketAns/PacketAnsUnique/PacketAdd/PacketAddUnique).
// NOTE: CacheRecordRmv calls AnswerQuestionWithResourceRecord which can call a user callback,
// which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void CacheRecordRmv(mDNS *const m, CacheRecord *rr)
{
	if (m->CurrentQuestion) LogMsg("CacheRecordRmv ERROR m->CurrentQuestion already set");
	m->CurrentQuestion = m->Questions;
	while (m->CurrentQuestion && m->CurrentQuestion != m->NewQuestions)
	{
		DNSQuestion *q = m->CurrentQuestion;
		m->CurrentQuestion = q->next;
		if (ResourceRecordAnswersQuestion(&rr->resrec, q))
		{
			verbosedebugf("CacheRecordRmv %p %##s (%s)", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
			if (q->CurrentAnswers == 0)
				LogMsg("CacheRecordRmv ERROR: How can CurrentAnswers already be zero for %p %##s (%s)?", q, q->qname.c, DNSTypeName(q->qtype));
			else
			{
				q->CurrentAnswers--;
				if (rr->resrec.rdlength > SmallRecordLimit) q->LargeAnswers--;
				if (rr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) q->UniqueAnswers--;
			}
			if (q->CurrentAnswers == 0)
			{
				debugf("CacheRecordRmv: Zero current answers for %##s (%s); will reconfirm antecedents", q->qname.c, DNSTypeName(q->qtype));
				ReconfirmAntecedents(m, q);
			}
			AnswerQuestionWithResourceRecord(m, q, rr, mDNSfalse);
			// MUST NOT dereference q again after calling AnswerQuestionWithResourceRecord()
		}
	}
	m->CurrentQuestion = mDNSNULL;
}

mDNSlocal void ReleaseCacheRR(mDNS *const m, CacheRecord *r)
{
	if (r->resrec.rdata && r->resrec.rdata != (RData*)&r->rdatastorage)
		mDNSPlatformMemFree(r->resrec.rdata);
	r->resrec.rdata = mDNSNULL;
	r->next = m->rrcache_free;
	m->rrcache_free = r;
	m->rrcache_totalused--;
}

mDNSlocal void CheckCacheExpiration(mDNS *const m, mDNSu32 slot)
{
	CacheRecord **rp = &(m->rrcache_hash[slot]);

	if (m->lock_rrcache) { LogMsg("CheckCacheExpiration ERROR! Cache already locked!"); return; }
	m->lock_rrcache = 1;

	while (*rp)
	{
		CacheRecord *const rr = *rp;
		mDNSs32 event = RRExpireTime(rr);
		if (m->timenow - event >= 0)	// If expired, delete it
		{
			*rp = rr->next;				// Cut it from the list
			verbosedebugf("CheckCacheExpiration: Deleting %s", GetRRDisplayString(m, rr));
			if (rr->CRActiveQuestion)	// If this record has one or more active questions, tell them it's going away
			{
				CacheRecordRmv(m, rr);
				m->rrcache_active--;
			}
			m->rrcache_used[slot]--;
			ReleaseCacheRR(m, rr);
		}
		else							// else, not expired; see if we need to query
		{
			if (rr->CRActiveQuestion && rr->UnansweredQueries < MaxUnansweredQueries)
			{
				if (m->timenow - rr->NextRequiredQuery < 0)		// If not yet time for next query
					event = rr->NextRequiredQuery;				// then just record when we want the next query
				else											// else trigger our question to go out now
				{
					// Set NextScheduledQuery to timenow so that SendQueries() will run.
					// SendQueries() will see that we have records close to expiration, and send FEQs for them.
					m->NextScheduledQuery = m->timenow;
					// After sending the query we'll increment UnansweredQueries and call SetNextCacheCheckTime(),
					// which will correctly update m->NextCacheCheck for us
					event = m->timenow + 0x3FFFFFFF;
				}
			}
			if (m->NextCacheCheck - (event + CacheCheckGracePeriod(rr)) > 0)
				m->NextCacheCheck = (event + CacheCheckGracePeriod(rr));
			rp = &rr->next;
		}
	}
	if (m->rrcache_tail[slot] != rp) debugf("CheckCacheExpiration: Updating m->rrcache_tail[%d] from %p to %p", slot, m->rrcache_tail[slot], rp);
	m->rrcache_tail[slot] = rp;
	m->lock_rrcache = 0;
}

mDNSlocal void AnswerNewQuestion(mDNS *const m)
{
	mDNSBool ShouldQueryImmediately = mDNStrue;
	CacheRecord *rr;
	DNSQuestion *q = m->NewQuestions;		// Grab the question we're going to answer
	mDNSu32 slot = HashSlot(&q->qname);

	verbosedebugf("AnswerNewQuestion: Answering %##s (%s)", q->qname.c, DNSTypeName(q->qtype));

	CheckCacheExpiration(m, slot);
	m->NewQuestions = q->next;				// Advance NewQuestions to the next *after* calling CheckCacheExpiration();

	if (m->lock_rrcache) LogMsg("AnswerNewQuestion ERROR! Cache already locked!");
	// This should be safe, because calling the client's question callback may cause the
	// question list to be modified, but should not ever cause the rrcache list to be modified.
	// If the client's question callback deletes the question, then m->CurrentQuestion will
	// be advanced, and we'll exit out of the loop
	m->lock_rrcache = 1;
	if (m->CurrentQuestion) LogMsg("AnswerNewQuestion ERROR m->CurrentQuestion already set");
	m->CurrentQuestion = q;		// Indicate which question we're answering, so we'll know if it gets deleted
	for (rr=m->rrcache_hash[slot]; rr; rr=rr->next)
		if (ResourceRecordAnswersQuestion(&rr->resrec, q))
		{
			// SecsSinceRcvd is whole number of elapsed seconds, rounded down
			mDNSu32 SecsSinceRcvd = ((mDNSu32)(m->timenow - rr->TimeRcvd)) / mDNSPlatformOneSecond;
			if (rr->resrec.rroriginalttl <= SecsSinceRcvd)
			{
				LogMsg("AnswerNewQuestion: How is rr->resrec.rroriginalttl %lu <= SecsSinceRcvd %lu for %##s (%s)",
				       rr->resrec.rroriginalttl, SecsSinceRcvd, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
				continue;	// Go to next one in loop
			}

			// If this record set is marked unique, then that means we can reasonably assume we have the whole set
			// -- we don't need to rush out on the network and query immediately to see if there are more answers out there
			if (rr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) ShouldQueryImmediately = mDNSfalse;
			q->CurrentAnswers++;
			if (rr->resrec.rdlength > SmallRecordLimit) q->LargeAnswers++;
			if (rr->resrec.RecordType & kDNSRecordTypePacketUniqueMask) q->UniqueAnswers++;
			AnswerQuestionWithResourceRecord(m, q, rr, mDNStrue);
			// MUST NOT dereference q again after calling AnswerQuestionWithResourceRecord()
			if (m->CurrentQuestion != q) break;		// If callback deleted q, then we're finished here
		}
		else if (RRTypeIsAddressType(rr->resrec.rrtype) && RRTypeIsAddressType(q->qtype))
			if (rr->resrec.namehash == q->qnamehash && SameDomainName(&rr->resrec.name, &q->qname))
				ShouldQueryImmediately = mDNSfalse;

	if (ShouldQueryImmediately && m->CurrentQuestion == q)
	{
		q->ThisQInterval = InitialQuestionInterval;
		q->LastQTime = m->timenow - q->ThisQInterval;
		m->NextScheduledQuery = m->timenow;
	}
	m->CurrentQuestion = mDNSNULL;
	m->lock_rrcache = 0;
}

mDNSlocal void AnswerLocalOnlyQuestionWithResourceRecord(mDNS *const m, DNSQuestion *q, AuthRecord *rr, mDNSBool AddRecord)
{
	// Indicate that we've given at least one positive answer for this record, so we should be prepared to send a goodbye for it
	if (AddRecord) rr->AnnounceCount = InitialAnnounceCount - 1;
	m->mDNS_reentrancy++; // Increment to allow client to legally make mDNS API calls from the callback
	if (q->QuestionCallback)
		q->QuestionCallback(m, q, &rr->resrec, AddRecord);
	m->mDNS_reentrancy--; // Decrement to block mDNS API calls again
}

mDNSlocal void AnswerNewLocalOnlyQuestion(mDNS *const m)
{
	DNSQuestion *q = m->NewLocalOnlyQuestions;		// Grab the question we're going to answer
	m->NewLocalOnlyQuestions = q->next;				// Advance NewQuestions to the next (if any)

	debugf("AnswerNewLocalOnlyQuestion: Answering %##s (%s)", q->qname.c, DNSTypeName(q->qtype));

	if (m->CurrentQuestion) LogMsg("AnswerNewQuestion ERROR m->CurrentQuestion already set");
	m->CurrentQuestion = q;		// Indicate which question we're answering, so we'll know if it gets deleted

	m->CurrentRecord = m->LocalOnlyRecords;
	while (m->CurrentRecord && m->CurrentRecord != m->NewLocalOnlyRecords)
	{
		AuthRecord *rr = m->CurrentRecord;
		m->CurrentRecord = rr->next;
		if (ResourceRecordAnswersQuestion(&rr->resrec, q))
		{
			AnswerLocalOnlyQuestionWithResourceRecord(m, q, rr, mDNStrue);
			// MUST NOT dereference q again after calling AnswerLocalOnlyQuestionWithResourceRecord()
			if (m->CurrentQuestion != q) break;		// If callback deleted q, then we're finished here
		}
	}

	m->CurrentQuestion = mDNSNULL;
}

mDNSlocal void AnswerLocalOnlyQuestions(mDNS *const m, AuthRecord *rr, mDNSBool AddRecord)
{
	if (m->CurrentQuestion) LogMsg("AnswerLocalOnlyQuestions ERROR m->CurrentQuestion already set");
	m->CurrentQuestion = m->LocalOnlyQuestions;
	while (m->CurrentQuestion && m->CurrentQuestion != m->NewLocalOnlyQuestions)
	{
		DNSQuestion *q = m->CurrentQuestion;
		m->CurrentQuestion = q->next;
		if (ResourceRecordAnswersQuestion(&rr->resrec, q))
		{
			debugf("AnswerLocalOnlyQuestions %p %##s (%s) %lu", rr, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), rr->resrec.rroriginalttl);
			AnswerLocalOnlyQuestionWithResourceRecord(m, q, rr, AddRecord);
			// MUST NOT dereference q again after calling AnswerQuestionWithResourceRecord()
		}
	}
	m->CurrentQuestion = mDNSNULL;
}

mDNSlocal void DiscardLocalOnlyRecords(mDNS *const m)
{
	AuthRecord *rr = m->LocalOnlyRecords;
	while (rr)
	{
		if (rr->resrec.RecordType == kDNSRecordTypeDeregistering)
		{ AnswerLocalOnlyQuestions(m, rr, mDNSfalse); CompleteDeregistration(m, rr); return; }
		if (rr->ProbeCount) { mDNS_Deregister_internal(m, rr, mDNS_Dereg_conflict); return; }
		rr=rr->next;
	}
	m->DiscardLocalOnlyRecords = mDNSfalse;
}

mDNSlocal void AnswerForNewLocalOnlyRecords(mDNS *const m)
{
	AuthRecord *rr = m->NewLocalOnlyRecords;
	m->NewLocalOnlyRecords = m->NewLocalOnlyRecords->next;
	AnswerLocalOnlyQuestions(m, rr, mDNStrue);
}

mDNSlocal CacheRecord *GetFreeCacheRR(mDNS *const m, mDNSu16 RDLength)
{
	CacheRecord *r = mDNSNULL;

	if (m->lock_rrcache) { LogMsg("GetFreeCacheRR ERROR! Cache already locked!"); return(mDNSNULL); }
	m->lock_rrcache = 1;
	
	// If we have no free records, ask the client layer to give us some more memory
	if (!m->rrcache_free && m->MainCallback)
	{
		if (m->rrcache_totalused != m->rrcache_size)
			LogMsg("GetFreeCacheRR: count mismatch: m->rrcache_totalused %lu != m->rrcache_size %lu",
			       m->rrcache_totalused, m->rrcache_size);
		
		// We don't want to be vulnerable to a malicious attacker flooding us with an infinite
		// number of bogus records so that we keep growing our cache until the machine runs out of memory.
		// To guard against this, if we're actively using less than 1/32 of our cache, then we
		// purge all the unused records and recycle them, instead of allocating more memory.
		if (m->rrcache_size >= 512 && m->rrcache_size / 32 > m->rrcache_active)
			debugf("Possible denial-of-service attack in progress: m->rrcache_size %lu; m->rrcache_active %lu",
			       m->rrcache_size, m->rrcache_active);
		else
			m->MainCallback(m, mStatus_GrowCache);
	}
	
	// If we still have no free records, recycle all the records we can.
	// Enumerating the entire cache is moderately expensive, so when we do it, we reclaim all the records we can in one pass.
	if (!m->rrcache_free)
	{
#if MDNS_DEBUGMSGS
		mDNSu32 oldtotalused = m->rrcache_totalused;
#endif
		mDNSu32 slot;
		for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
		{
			CacheRecord **rp = &(m->rrcache_hash[slot]);
			while (*rp)
			{
				// Records that answer still-active questions are not candidates for deletion
				if ((*rp)->CRActiveQuestion)
					rp=&(*rp)->next;
				else
				{
					CacheRecord *rr = *rp;
					*rp = (*rp)->next;			// Cut record from list
					m->rrcache_used[slot]--;	// Decrement counts
					ReleaseCacheRR(m, rr);
				}
			}
			if (m->rrcache_tail[slot] != rp) debugf("GetFreeCacheRR: Updating m->rrcache_tail[%d] from %p to %p", slot, m->rrcache_tail[slot], rp);
			m->rrcache_tail[slot] = rp;
		}
#if MDNS_DEBUGMSGS
		debugf("Clear unused records; m->rrcache_totalused was %lu; now %lu", oldtotalused, m->rrcache_totalused);
#endif
	}
	
	if (m->rrcache_free)	// If there are records in the free list, take one
	{
		r = m->rrcache_free;
		m->rrcache_free = r->next;
	}

	if (r)
	{
		if (++m->rrcache_totalused >= m->rrcache_report)
		{
			debugf("RR Cache now using %ld records", m->rrcache_totalused);
			if (m->rrcache_report < 100) m->rrcache_report += 10;
			else                         m->rrcache_report += 100;
		}
		mDNSPlatformMemZero(r, sizeof(*r));
		r->resrec.rdata = (RData*)&r->rdatastorage;		// By default, assume we're usually going to be using local storage
	
		if (RDLength > InlineCacheRDSize)		// If RDLength is too big, allocate extra storage
		{
			r->resrec.rdata = (RData*)mDNSPlatformMemAllocate(sizeofRDataHeader + RDLength);
			if (r->resrec.rdata) r->resrec.rdata->MaxRDLength = r->resrec.rdlength = RDLength;
			else { ReleaseCacheRR(m, r); r = mDNSNULL; }
		}
	}

	m->lock_rrcache = 0;

	return(r);
}

mDNSlocal void PurgeCacheResourceRecord(mDNS *const m, CacheRecord *rr)
{
	// Make sure we mark this record as thoroughly expired -- we don't ever want to give
	// a positive answer using an expired record (e.g. from an interface that has gone away).
	// We don't want to clear CRActiveQuestion here, because that would leave the record subject to
	// summary deletion without giving the proper callback to any questions that are monitoring it.
	// By setting UnansweredQueries to MaxUnansweredQueries we ensure it won't trigger any further expiration queries.
	rr->TimeRcvd          = m->timenow - mDNSPlatformOneSecond * 60;
	rr->UnansweredQueries = MaxUnansweredQueries;
	rr->resrec.rroriginalttl     = 0;
	SetNextCacheCheckTime(m, rr);
}

mDNSexport void mDNS_Lock(mDNS *const m)
{
	// MUST grab the platform lock FIRST!
	mDNSPlatformLock(m);

	// Normally, mDNS_reentrancy is zero and so is mDNS_busy
	// However, when we call a client callback mDNS_busy is one, and we increment mDNS_reentrancy too
	// If that client callback does mDNS API calls, mDNS_reentrancy and mDNS_busy will both be one
	// If mDNS_busy != mDNS_reentrancy that's a bad sign
	if (m->mDNS_busy != m->mDNS_reentrancy)
		LogMsg("mDNS_Lock: Locking failure! mDNS_busy (%ld) != mDNS_reentrancy (%ld)", m->mDNS_busy, m->mDNS_reentrancy);

	// If this is an initial entry into the mDNSCore code, set m->timenow
	// else, if this is a re-entrant entry into the mDNSCore code, m->timenow should already be set
	if (m->mDNS_busy == 0)
	{
		if (m->timenow)
			LogMsg("mDNS_Lock: m->timenow already set (%ld/%ld)", m->timenow, mDNSPlatformTimeNow() + m->timenow_adjust);
		m->timenow = mDNSPlatformTimeNow() + m->timenow_adjust;
		if (m->timenow == 0) m->timenow = 1;
	}
	else if (m->timenow == 0)
	{
		LogMsg("mDNS_Lock: m->mDNS_busy is %ld but m->timenow not set", m->mDNS_busy);
		m->timenow = mDNSPlatformTimeNow() + m->timenow_adjust;
		if (m->timenow == 0) m->timenow = 1;
	}

	if (m->timenow_last - m->timenow > 0)
	{
		m->timenow_adjust += m->timenow_last - m->timenow;
		LogMsg("mDNSPlatformTimeNow went backwards by %ld ticks; setting correction factor to %ld", m->timenow_last - m->timenow, m->timenow_adjust);
		m->timenow = m->timenow_last;
	}
	m->timenow_last = m->timenow;

	// Increment mDNS_busy so we'll recognise re-entrant calls
	m->mDNS_busy++;
}

mDNSlocal mDNSs32 GetNextScheduledEvent(const mDNS *const m)
{
	mDNSs32 e = m->timenow + kNextScheduledTime;
	LogMsg("GetNextScheduledEvent e=%d, m->timenow= %d\n", e, m->timenow);
	if (m->mDNSPlatformStatus != mStatus_NoError || m->SleepState) return(e);
	LogMsg("GetNextScheduledEvent m->NewQuestions=%d, m->NewLocalOnlyQuestions=%d, m->NewLocalOnlyRecords=%d, m->DiscardLocalOnlyRecords=%d\n",
		m->NewQuestions, m->NewLocalOnlyQuestions, m->NewLocalOnlyRecords, m->DiscardLocalOnlyRecords);
	if (m->NewQuestions)            return(m->timenow);
	if (m->NewLocalOnlyQuestions)   return(m->timenow);
	if (m->NewLocalOnlyRecords)     return(m->timenow);
	if (m->DiscardLocalOnlyRecords) return(m->timenow);
	LogMsg("GetNextScheduledEvent SuppressSending=%d\n", m->SuppressSending);
	if (m->SuppressSending)         return(m->SuppressSending);
	if (e - m->NextCacheCheck        > 0) e = m->NextCacheCheck;
	if (e - m->NextScheduledQuery    > 0) e = m->NextScheduledQuery;
	if (e - m->NextScheduledProbe    > 0) e = m->NextScheduledProbe;
	if (e - m->NextScheduledResponse > 0) e = m->NextScheduledResponse;
	LogMsg("GetNextScheduledEvent m->rrcache_size=%d, m->NextCacheCheck=%d, m->NextScheduledQuery=%d, m->NextScheduledProbe=%d, m->NextScheduledResponse=%d\n",
		m->rrcache_size, m->NextCacheCheck, m->NextScheduledQuery, m->NextScheduledProbe, m->NextScheduledResponse);
	return(e);
}

mDNSexport void mDNS_Unlock(mDNS *const m)
{
	// Decrement mDNS_busy
	m->mDNS_busy--;
	
	// Check for locking failures
	if (m->mDNS_busy != m->mDNS_reentrancy)
		LogMsg("mDNS_Unlock: Locking failure! mDNS_busy (%ld) != mDNS_reentrancy (%ld)", m->mDNS_busy, m->mDNS_reentrancy);

	// If this is a final exit from the mDNSCore code, set m->NextScheduledEvent and clear m->timenow
	if (m->mDNS_busy == 0)
	{
		m->NextScheduledEvent = GetNextScheduledEvent(m);
		LogMsg("mDNS_Unlock: NextScheduledEvent %d\n", m->NextScheduledEvent);
		if (m->timenow == 0) LogMsg("mDNS_Unlock: ERROR! m->timenow aready zero");
		m->timenow = 0;
	}

	// MUST release the platform lock LAST!
	mDNSPlatformUnlock(m);
}

mDNSexport mDNSs32 mDNS_Execute(mDNS *const m)
{
	mDNS_Lock(m);	// Must grab lock before trying to read m->timenow

	LogMsg("mDNS_Execute: m->timenow=%d, m->NextScheduledEvent=%d\n", m->timenow, m->NextScheduledEvent);
	if (m->timenow - m->NextScheduledEvent >= 0)
	{
		int i;

		verbosedebugf("mDNS_Execute");
		if (m->CurrentQuestion) LogMsg("mDNS_Execute: ERROR! m->CurrentQuestion already set");
	
		// 1. If we're past the probe suppression time, we can clear it
		if (m->SuppressProbes && m->timenow - m->SuppressProbes >= 0) m->SuppressProbes = 0;
	
		// 2. If it's been more than ten seconds since the last probe failure, we can clear the counter
		if (m->NumFailedProbes && m->timenow - m->ProbeFailTime >= mDNSPlatformOneSecond * 10) m->NumFailedProbes = 0;
		
		// 3. Purge our cache of stale old records
		if (m->rrcache_size && m->timenow - m->NextCacheCheck >= 0)
		{
			mDNSu32 slot;
			m->NextCacheCheck = m->timenow + 0x3FFFFFFF;
			for (slot = 0; slot < CACHE_HASH_SLOTS; slot++) CheckCacheExpiration(m, slot);
		}
	
		// 4. See if we can answer any of our new local questions from the cache
		for (i=0; m->NewQuestions && i<1000; i++) AnswerNewQuestion(m);
		if (i >= 1000) debugf("mDNS_Execute: AnswerNewQuestion exceeded loop limit");
		
		for (i=0; m->DiscardLocalOnlyRecords && i<1000; i++) DiscardLocalOnlyRecords(m);
		if (i >= 1000) debugf("mDNS_Execute: DiscardLocalOnlyRecords exceeded loop limit");

		for (i=0; m->NewLocalOnlyQuestions && i<1000; i++) AnswerNewLocalOnlyQuestion(m);
		if (i >= 1000) debugf("mDNS_Execute: AnswerNewLocalOnlyQuestion exceeded loop limit");

		for (i=0; m->NewLocalOnlyRecords && i<1000; i++) AnswerForNewLocalOnlyRecords(m);
		if (i >= 1000) debugf("mDNS_Execute: AnswerLocalOnlyQuestions exceeded loop limit");

		// 5. See what packets we need to send
		if (m->mDNSPlatformStatus != mStatus_NoError || m->SleepState) DiscardDeregistrations(m);
		else if (m->SuppressSending == 0 || m->timenow - m->SuppressSending >= 0)
		{
			// If the platform code is ready, and we're not suppressing packet generation right now
			// then send our responses, probes, and questions.
			// We check the cache first, because there might be records close to expiring that trigger questions to refresh them
			// We send queries next, because there might be final-stage probes that complete their probing here, causing
			// them to advance to announcing state, and we want those to be included in any announcements we send out.
			// Finally, we send responses, including the previously mentioned records that just completed probing
			m->SuppressSending = 0;
	
			// 6. Send Query packets. This may cause some probing records to advance to announcing state
			if (m->timenow - m->NextScheduledQuery >= 0 || m->timenow - m->NextScheduledProbe >= 0) SendQueries(m);
			if (m->timenow - m->NextScheduledQuery >= 0)
			{
				LogMsg("mDNS_Execute: SendQueries didn't send all its queries; will try again in one second");
				m->NextScheduledQuery = m->timenow + mDNSPlatformOneSecond;
			}
			if (m->timenow - m->NextScheduledProbe >= 0)
			{
				LogMsg("mDNS_Execute: SendQueries didn't send all its probes; will try again in one second");
				m->NextScheduledProbe = m->timenow + mDNSPlatformOneSecond;
			}
	
			// 7. Send Response packets, including probing records just advanced to announcing state
			if (m->timenow - m->NextScheduledResponse >= 0) SendResponses(m);
			if (m->timenow - m->NextScheduledResponse >= 0)
			{
				LogMsg("mDNS_Execute: SendResponses didn't send all its responses; will try again in one second");
				m->NextScheduledResponse = m->timenow + mDNSPlatformOneSecond;
			}
		}

		m->RandomQueryDelay = 0;	// Clear m->RandomQueryDelay, ready to pick a new different value, when necessary
	}

	// Note about multi-threaded systems:
	// On a multi-threaded system, some other thread could run right after the mDNS_Unlock(),
	// performing mDNS API operations that change our next scheduled event time.
	//
	// On multi-threaded systems (like the current Windows implementation) that have a single main thread
	// calling mDNS_Execute() (and other threads allowed to call mDNS API routines) it is the responsibility
	// of the mDNSPlatformUnlock() routine to signal some kind of stateful condition variable that will
	// signal whatever blocking primitive the main thread is using, so that it will wake up and execute one
	// more iteration of its loop, and immediately call mDNS_Execute() again. The signal has to be stateful
	// in the sense that if the main thread has not yet entered its blocking primitive, then as soon as it
	// does, the state of the signal will be noticed, causing the blocking primitive to return immediately
	// without blocking. This avoids the race condition between the signal from the other thread arriving
	// just *before* or just *after* the main thread enters the blocking primitive.
	//
	// On multi-threaded systems (like the current Mac OS 9 implementation) that are entirely timer-driven,
	// with no main mDNS_Execute() thread, it is the responsibility of the mDNSPlatformUnlock() routine to
	// set the timer according to the m->NextScheduledEvent value, and then when the timer fires, the timer
	// callback function should call mDNS_Execute() (and ignore the return value, which may already be stale
	// by the time it gets to the timer callback function).

	mDNS_Unlock(m);		// Calling mDNS_Unlock is what gives m->NextScheduledEvent its new value
	return(m->NextScheduledEvent);
}
#if 0
// Call mDNSCoreMachineSleep(m, mDNStrue) when the machine is about to go to sleep.
// Call mDNSCoreMachineSleep(m, mDNSfalse) when the machine is has just woken up.
// Normally, the platform support layer below mDNSCore should call this, not the client layer above.
// Note that sleep/wake calls do not have to be paired one-for-one; it is acceptable to call
// mDNSCoreMachineSleep(m, mDNSfalse) any time there is reason to believe that the machine may have just
// found itself in a new network environment. For example, if the Ethernet hardware indicates that the
// cable has just been connected, the platform support layer should call mDNSCoreMachineSleep(m, mDNSfalse)
// to make mDNSCore re-issue its outstanding queries, probe for record uniqueness, etc.
// While it is safe to call mDNSCoreMachineSleep(m, mDNSfalse) at any time, it does cause extra network
// traffic, so it should only be called when there is legitimate reason to believe the machine
// may have become attached to a new network.
mDNSexport void mDNSCoreMachineSleep(mDNS *const m, mDNSBool sleepstate)
{
	AuthRecord *rr;

	mDNS_Lock(m);

	m->SleepState = sleepstate;
	LogMsg("mDNSResponder %s at %ld", sleepstate ? "Sleeping" : "Waking", m->timenow);

	if (sleepstate)
	{
		// Mark all the records we need to deregister and send them
		for (rr = m->ResourceRecords; rr; rr=rr->next)
			if (rr->resrec.RecordType == kDNSRecordTypeShared && rr->AnnounceCount < InitialAnnounceCount)
				rr->ImmedAnswer = mDNSInterfaceMark;
		SendResponses(m);
	}
	else
	{
		DNSQuestion *q;
		mDNSu32 slot;
		CacheRecord *cr;

		// 1. Retrigger all our questions
		for (q = m->Questions; q; q=q->next)				// Scan our list of questions
			if (ActiveQuestion(q))
			{
				q->ThisQInterval = InitialQuestionInterval;	// MUST be > zero for an active question
				q->LastQTime     = m->timenow - q->ThisQInterval;
				q->RecentAnswers = 0;
				ExpireDupSuppressInfo(q->DupSuppress, m->timenow);
				m->NextScheduledQuery = m->timenow;
			}

		// 2. Re-validate our cache records
		m->NextCacheCheck  = m->timenow;
		for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
			for (cr = m->rrcache_hash[slot]; cr; cr=cr->next)
				mDNS_Reconfirm_internal(m, cr, kDefaultReconfirmTimeForCableDisconnect);

		// 3. Retrigger probing and announcing for all our authoritative records
		for (rr = m->ResourceRecords; rr; rr=rr->next)
		{
			if (rr->resrec.RecordType == kDNSRecordTypeVerified && !rr->DependentOn) rr->resrec.RecordType = kDNSRecordTypeUnique;
			rr->ProbeCount        = DefaultProbeCountForRecordType(rr->resrec.RecordType);
			if (rr->AnnounceCount < ReannounceCount)
				rr->AnnounceCount = ReannounceCount;
			rr->ThisAPInterval    = DefaultAPIntervalForRecordType(rr->resrec.RecordType);
			InitializeLastAPTime(m, rr);
		}

	}

	mDNS_Unlock(m);
}
#endif
// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Packet Reception Functions
#endif

mDNSlocal void AddRecordToResponseList(AuthRecord ***nrpp, AuthRecord *rr, AuthRecord *add)
{
	if (rr->NextResponse == mDNSNULL && *nrpp != &rr->NextResponse)
	{
		**nrpp = rr;
		// NR_AdditionalTo must point to a record with NR_AnswerTo set (and not NR_AdditionalTo)
		// If 'add' does not meet this requirement, then follow its NR_AdditionalTo pointer to a record that does
		// The referenced record will definitely be acceptable (by recursive application of this rule)
		if (add && add->NR_AdditionalTo) add = add->NR_AdditionalTo;
		rr->NR_AdditionalTo = add;
		*nrpp = &rr->NextResponse;
	}
	debugf("AddRecordToResponseList: %##s (%s) already in list", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
}

#define MustSendRecord(RR) ((RR)->NR_AnswerTo || (RR)->NR_AdditionalTo)

mDNSlocal mDNSu8 *GenerateUnicastResponse(const DNSMessage *const query, const mDNSu8 *const end,
					  const mDNSInterfaceID InterfaceID, mDNSBool LegacyQuery, DNSMessage *const response, AuthRecord *ResponseRecords)
{
	mDNSu8          *responseptr     = response->data;
	const mDNSu8    *const limit     = response->data + sizeof(response->data);
	const mDNSu8    *ptr             = query->data;
	AuthRecord  *rr;
	mDNSu32          maxttl = 0x70000000;
	int i;

	// Initialize the response fields so we can answer the questions
	InitializeDNSMessage(&response->h, query->h.id, ResponseFlags);

	// ***
	// *** 1. Write out the list of questions we are actually going to answer with this packet
	// ***
	if (LegacyQuery)
	{
		DNSQuestion *q = tls_mem_alloc(sizeof(DNSQuestion));
		maxttl = 10;
		if(q)
		{
			memset(q, 0, sizeof(DNSQuestion));
			for (i=0; i<query->h.numQuestions; i++)						// For each question...
			{
				ptr = getQuestion(query, ptr, end, InterfaceID, q);	// get the question...
				if (!ptr) { tls_mem_free(q); return(mDNSNULL); }
		
				for (rr=ResponseRecords; rr; rr=rr->NextResponse)		// and search our list of proposed answers
				{
					if (rr->NR_AnswerTo == ptr)							// If we're going to generate a record answering this question
					{												// then put the question in the question section
						responseptr = putQuestion(response, responseptr, limit, &q->qname, q->qtype, q->qclass);
						if (!responseptr) { debugf("GenerateUnicastResponse: Ran out of space for questions!"); tls_mem_free(q); return(mDNSNULL); }
						break;		// break out of the ResponseRecords loop, and go on to the next question
					}
				}
			}
			tls_mem_free(q);
		}
		if (response->h.numQuestions == 0) { LogMsg("GenerateUnicastResponse: ERROR! Why no questions?"); return(mDNSNULL); }
	}

	// ***
	// *** 2. Write Answers
	// ***
	for (rr=ResponseRecords; rr; rr=rr->NextResponse)
		if (rr->NR_AnswerTo)
		{
			mDNSu8 *p = PutResourceRecordCappedTTL(response, responseptr, &response->h.numAnswers, &rr->resrec, maxttl);
			if (p) responseptr = p;
			else { debugf("GenerateUnicastResponse: Ran out of space for answers!"); response->h.flags.b[0] |= kDNSFlag0_TC; }
		}

	// ***
	// *** 3. Write Additionals
	// ***
	for (rr=ResponseRecords; rr; rr=rr->NextResponse)
		if (rr->NR_AdditionalTo && !rr->NR_AnswerTo)
		{
			mDNSu8 *p = PutResourceRecordCappedTTL(response, responseptr, &response->h.numAdditionals, &rr->resrec, maxttl);
			if (p) responseptr = p;
			else debugf("GenerateUnicastResponse: No more space for additionals");
		}

	return(responseptr);
}

// AuthRecord *our is our Resource Record
// CacheRecord *pkt is the Resource Record from the response packet we've witnessed on the network
// Returns 0 if there is no conflict
// Returns +1 if there was a conflict and we won
// Returns -1 if there was a conflict and we lost and have to rename
mDNSlocal int CompareRData(AuthRecord *our, CacheRecord *pkt)
{
	mDNSu8 *ourdata = NULL, *ourptr = ourdata, *ourend;
	mDNSu8 *pktdata = NULL, *pktptr = pktdata, *pktend;
	if (!our) { LogMsg("CompareRData ERROR: our is NULL"); return(+1); }
	if (!pkt) { LogMsg("CompareRData ERROR: pkt is NULL"); return(+1); }

	ourdata = tls_mem_alloc(512);
	if(!ourdata) return (0);
	pktdata = ourdata + 256;
	memset(ourdata, 0, 512);
	ourend = putRData(mDNSNULL, ourdata, ourdata + 256, &our->resrec);
	pktend = putRData(mDNSNULL, pktdata, pktdata + 256, &pkt->resrec);
	while (ourptr < ourend && pktptr < pktend && *ourptr == *pktptr) { ourptr++; pktptr++; }
	if (ourptr >= ourend && pktptr >= pktend) { tls_mem_free(ourdata); return(0); }		// If data identical, not a conflict

	if (ourptr >= ourend) { tls_mem_free(ourdata); return(-1); }									// Our data ran out first; We lost
	if (pktptr >= pktend) { tls_mem_free(ourdata); return(+1); }									// Packet data ran out first; We won
	if (*pktptr > *ourptr) { tls_mem_free(ourdata); return(-1); }								// Our data is numerically lower; We lost
	if (*pktptr < *ourptr) { tls_mem_free(ourdata); return(+1); }								// Packet data is numerically lower; We won
	
	debugf("CompareRData: How did we get here?");
	tls_mem_free(ourdata);
	return(-1);
}

// See if we have an authoritative record that's identical to this packet record,
// whose canonical DependentOn record is the specified master record.
// The DependentOn pointer is typically used for the TXT record of service registrations
// It indicates that there is no inherent conflict detection for the TXT record
// -- it depends on the SRV record to resolve name conflicts
// If we find any identical ResourceRecords in our authoritative list, then follow their DependentOn
// pointer chain (if any) to make sure we reach the canonical DependentOn record
// If the record has no DependentOn, then just return that record's pointer
// Returns NULL if we don't have any local RRs that are identical to the one from the packet
mDNSlocal mDNSBool MatchDependentOn(const mDNS *const m, const CacheRecord *const pktrr, const AuthRecord *const master)
{
	const AuthRecord *r1;
	for (r1 = m->ResourceRecords; r1; r1=r1->next)
	{
		if (IdenticalResourceRecord(&r1->resrec, &pktrr->resrec))
		{
			const AuthRecord *r2 = r1;
			while (r2->DependentOn) r2 = r2->DependentOn;
			if (r2 == master) return(mDNStrue);
		}
	}
	for (r1 = m->DuplicateRecords; r1; r1=r1->next)
	{
		if (IdenticalResourceRecord(&r1->resrec, &pktrr->resrec))
		{
			const AuthRecord *r2 = r1;
			while (r2->DependentOn) r2 = r2->DependentOn;
			if (r2 == master) return(mDNStrue);
		}
	}
	return(mDNSfalse);
}

// Find the canonical RRSet pointer for this RR received in a packet.
// If we find any identical AuthRecord in our authoritative list, then follow its RRSet
// pointers (if any) to make sure we return the canonical member of this name/type/class
// Returns NULL if we don't have any local RRs that are identical to the one from the packet
mDNSlocal const AuthRecord *FindRRSet(const mDNS *const m, const CacheRecord *const pktrr)
{
	const AuthRecord *rr;
	for (rr = m->ResourceRecords; rr; rr=rr->next)
	{
		if (IdenticalResourceRecord(&rr->resrec, &pktrr->resrec))
		{
			while (rr->RRSet && rr != rr->RRSet) rr = rr->RRSet;
			return(rr);
		}
	}
	return(mDNSNULL);
}

// PacketRRConflict is called when we've received an RR (pktrr) which has the same name
// as one of our records (our) but different rdata.
// 1. If our record is not a type that's supposed to be unique, we don't care.
// 2a. If our record is marked as dependent on some other record for conflict detection, ignore this one.
// 2b. If the packet rr exactly matches one of our other RRs, and *that* record's DependentOn pointer
//     points to our record, ignore this conflict (e.g. the packet record matches one of our
//     TXT records, and that record is marked as dependent on 'our', its SRV record).
// 3. If we have some *other* RR that exactly matches the one from the packet, and that record and our record
//    are members of the same RRSet, then this is not a conflict.
mDNSlocal mDNSBool PacketRRConflict(const mDNS *const m, const AuthRecord *const our, const CacheRecord *const pktrr)
{
	const AuthRecord *ourset = our->RRSet ? our->RRSet : our;

	// If not supposed to be unique, not a conflict
	if (!(our->resrec.RecordType & kDNSRecordTypeUniqueMask)) return(mDNSfalse);

	// If a dependent record, not a conflict
	if (our->DependentOn || MatchDependentOn(m, pktrr, our)) return(mDNSfalse);

	// If the pktrr matches a member of ourset, not a conflict
	if (FindRRSet(m, pktrr) == ourset) return(mDNSfalse);

	// Okay, this is a conflict
	return(mDNStrue);
}

// NOTE: ResolveSimultaneousProbe calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void ResolveSimultaneousProbe(mDNS *const m, const DNSMessage *const query, const mDNSu8 *const end,
					DNSQuestion *q, AuthRecord *our)
{
	int i;
	const mDNSu8 *ptr = LocateAuthorities(query, end);
	mDNSBool FoundUpdate = mDNSfalse;
	LargeCacheRecord *pkt = tls_mem_alloc(sizeof(LargeCacheRecord));

	if(!pkt) goto exit;
	for (i = 0; i < query->h.numAuthorities; i++)
	{
		memset(pkt, 0, sizeof(LargeCacheRecord));
		ptr = GetLargeResourceRecord(m, query, ptr, end, q->InterfaceID, 0, pkt);
		if (!ptr) break;
		if (ResourceRecordAnswersQuestion(&pkt->r.resrec, q))
		{
			FoundUpdate = mDNStrue;
			if (PacketRRConflict(m, our, &pkt->r))
			{
				int result          = (int)our->resrec.rrclass - (int)pkt->r.resrec.rrclass;
				if (!result) result = (int)our->resrec.rrtype  - (int)pkt->r.resrec.rrtype;
				if (!result) result = CompareRData(our, &pkt->r);
				switch (result)
				{
				case  1:	debugf("ResolveSimultaneousProbe: %##s (%s): We won",  our->resrec.name.c, DNSTypeName(our->resrec.rrtype));
					break;
				case  0:	break;
				case -1:	debugf("ResolveSimultaneousProbe: %##s (%s): We lost", our->resrec.name.c, DNSTypeName(our->resrec.rrtype));
					mDNS_Deregister_internal(m, our, mDNS_Dereg_conflict);
					goto exit;
				}
			}
		}
	}
exit:
	if(pkt)
		tls_mem_free(pkt);
	if (!FoundUpdate)
		debugf("ResolveSimultaneousProbe: %##s (%s): No Update Record found", our->resrec.name.c, DNSTypeName(our->resrec.rrtype));
}

mDNSlocal CacheRecord *FindIdenticalRecordInCache(const mDNS *const m, ResourceRecord *pktrr)
{
	CacheRecord *rr;
	for (rr = m->rrcache_hash[HashSlot(&pktrr->name)]; rr; rr=rr->next)
		if (pktrr->InterfaceID == rr->resrec.InterfaceID && IdenticalResourceRecord(pktrr, &rr->resrec)) break;
	return(rr);
}

// ProcessQuery examines a received query to see if we have any answers to give
mDNSlocal mDNSu8 *ProcessQuery(mDNS *const m, const DNSMessage *const query, const mDNSu8 *const end,
			       const mDNSAddr *srcaddr, const mDNSInterfaceID InterfaceID, mDNSBool LegacyQuery, mDNSBool QueryWasMulticast,
			       DNSMessage *const response)
{
	AuthRecord  *ResponseRecords = mDNSNULL;
	AuthRecord **nrp             = &ResponseRecords;
	CacheRecord  *ExpectedAnswers = mDNSNULL;			// Records in our cache we expect to see updated
	CacheRecord **eap             = &ExpectedAnswers;
	DNSQuestion     *DupQuestions    = mDNSNULL;			// Our questions that are identical to questions in this packet
	DNSQuestion    **dqp             = &DupQuestions;
	mDNSs32          delayresponse   = 0;
	mDNSBool         HaveUnicastAnswer = mDNSfalse;
	const mDNSu8    *ptr             = query->data;
	mDNSu8          *responseptr     = mDNSNULL;
	AuthRecord  *rr, *rr2;
	int i;

	// If TC flag is set, it means we should expect that additional known answers may be coming in another packet.
	if (query->h.flags.b[0] & kDNSFlag0_TC) delayresponse = mDNSPlatformOneSecond;	// Divided by 50 = 20ms

	// ***
	// *** 1. Parse Question Section and mark potential answers
	// ***
	for (i=0; i<query->h.numQuestions; i++)						// For each question...
	{
		mDNSBool QuestionNeedsMulticastResponse;
		int NumAnswersForThisQuestion = 0;
		DNSQuestion *pktq, *q;
		pktq = tls_mem_alloc(sizeof(DNSQuestion));
		if (!pktq) goto exit;
		memset(pktq, 0, sizeof(DNSQuestion));
		ptr = getQuestion(query, ptr, end, InterfaceID, pktq);	// get the question...
		if (!ptr) { tls_mem_free(pktq); goto exit; }

		// The only queries that *need* a multicast response are:
		// * Queries sent via multicast
		// * from port 5353
		// * that don't have the kDNSQClass_UnicastResponse bit set
		// These queries need multicast responses because other clients will:
		// * suppress their own identical questions when they see these questions, and
		// * expire their cache records if they don't see the expected responses
		// For other queries, we may still choose to send the occasional multicast response anyway,
		// to keep our neighbours caches warm, and for ongoing conflict detection.
		QuestionNeedsMulticastResponse = QueryWasMulticast && !LegacyQuery && !(pktq->qclass & kDNSQClass_UnicastResponse);
		// Clear the UnicastResponse flag -- don't want to confuse the rest of the code that follows later
		pktq->qclass &= ~kDNSQClass_UnicastResponse;
		
		// Note: We use the m->CurrentRecord mechanism here because calling ResolveSimultaneousProbe
		// can result in user callbacks which may change the record list and/or question list.
		// Also note: we just mark potential answer records here, without trying to build the
		// "ResponseRecords" list, because we don't want to risk user callbacks deleting records
		// from that list while we're in the middle of trying to build it.
		if (m->CurrentRecord) LogMsg("ProcessQuery ERROR m->CurrentRecord already set");
		m->CurrentRecord = m->ResourceRecords;
		while (m->CurrentRecord)
		{
			rr = m->CurrentRecord;
			m->CurrentRecord = rr->next;
			if (ResourceRecordAnswersQuestion(&rr->resrec, pktq))
			{
				if (rr->resrec.RecordType == kDNSRecordTypeUnique)
					ResolveSimultaneousProbe(m, query, end, pktq, rr);
				else if (ResourceRecordIsValidAnswer(rr))
				{
					NumAnswersForThisQuestion++;
					// Notes:
					// NR_AnswerTo pointing into query packet means "answer via unicast"
					//                         (may also choose to do multicast as well)
					// NR_AnswerTo == ~0 means "definitely answer via multicast" (can't downgrade to unicast later)
					if (QuestionNeedsMulticastResponse)
					{
						// We only mark this question for sending if it is at least one second since the last time we multicast it
						// on this interface. If it is more than a second, or LastMCInterface is different, then we should multicast it.
						// This is to guard against the case where someone blasts us with queries as fast as they can.
						if (m->timenow - (rr->LastMCTime + mDNSPlatformOneSecond) >= 0 ||
						    (rr->LastMCInterface != mDNSInterfaceMark && rr->LastMCInterface != InterfaceID))
							rr->NR_AnswerTo = (mDNSu8*)~0;
					}
					else if (!rr->NR_AnswerTo) rr->NR_AnswerTo = ptr;
				}
			}
		}

		// We only do the following accelerated cache expiration processing and duplicate question suppression processing
		// for multicast queries with multicast responses.
		// For any query generating a unicast response we don't do this because we can't assume we will see the response
		if (QuestionNeedsMulticastResponse)
		{
			CacheRecord *rr;
			// If we couldn't answer this question, someone else might be able to,
			// so use random delay on response to reduce collisions
			if (NumAnswersForThisQuestion == 0) delayresponse = mDNSPlatformOneSecond;	// Divided by 50 = 20ms

			// Make a list indicating which of our own cache records we expect to see updated as a result of this query
			// Note: Records larger than 1K are not habitually multicast, so don't expect those to be updated
			for (rr = m->rrcache_hash[HashSlot(&pktq->qname)]; rr; rr=rr->next)
				if (ResourceRecordAnswersQuestion(&rr->resrec, pktq) && rr->resrec.rdlength <= SmallRecordLimit)
					if (!rr->NextInKAList && eap != &rr->NextInKAList)
					{
						*eap = rr;
						eap = &rr->NextInKAList;
						if (rr->MPUnansweredQ == 0 || m->timenow - rr->MPLastUnansweredQT >= mDNSPlatformOneSecond)
						{
							// Although MPUnansweredQ is only really used for multi-packet query processing,
							// we increment it for both single-packet and multi-packet queries, so that it stays in sync
							// with the MPUnansweredKA value, which by necessity is incremented for both query types.
							rr->MPUnansweredQ++;
							rr->MPLastUnansweredQT = m->timenow;
							rr->MPExpectingKA = mDNStrue;
						}
					}
	
			// Check if this question is the same as any of mine.
			// We only do this for non-truncated queries. Right now it would be too complicated to try
			// to keep track of duplicate suppression state between multiple packets, especially when we
			// can't guarantee to receive all of the Known Answer packets that go with a particular query.
			if (!(query->h.flags.b[0] & kDNSFlag0_TC))
				for (q = m->Questions; q; q=q->next)
					if (ActiveQuestion(q) && m->timenow - q->LastQTxTime > mDNSPlatformOneSecond / 4)
						if (!q->InterfaceID || q->InterfaceID == InterfaceID)
							if (q->NextInDQList == mDNSNULL && dqp != &q->NextInDQList)
								if (q->qtype == pktq->qtype && q->qclass == pktq->qclass && q->qnamehash == pktq->qnamehash && SameDomainName(&q->qname, &pktq->qname))
								{ *dqp = q; dqp = &q->NextInDQList; }
		}
		tls_mem_free(pktq);
	}

	// ***
	// *** 2. Now we can safely build the list of marked answers
	// ***
	for (rr = m->ResourceRecords; rr; rr=rr->next)				// Now build our list of potential answers
		if (rr->NR_AnswerTo)									// If we marked the record...
			AddRecordToResponseList(&nrp, rr, mDNSNULL);		// ... add it to the list

	// ***
	// *** 3. Add additional records
	// ***
	for (rr=ResponseRecords; rr; rr=rr->NextResponse)			// For each record we plan to put
	{
		// (Note: This is an "if", not a "while". If we add a record, we'll find it again
		// later in the "for" loop, and we will follow further "additional" links then.)
		if (rr->Additional1 && ResourceRecordIsValidInterfaceAnswer(rr->Additional1, InterfaceID))
			AddRecordToResponseList(&nrp, rr->Additional1, rr);

		if (rr->Additional2 && ResourceRecordIsValidInterfaceAnswer(rr->Additional2, InterfaceID))
			AddRecordToResponseList(&nrp, rr->Additional2, rr);
		
		// For SRV records, automatically add the Address record(s) for the target host
		if (rr->resrec.rrtype == kDNSType_SRV)
			for (rr2=m->ResourceRecords; rr2; rr2=rr2->next)					// Scan list of resource records
				if (RRIsAddressType(rr2) &&										// For all address records (A/AAAA) ...
				    ResourceRecordIsValidInterfaceAnswer(rr2, InterfaceID) &&	// ... which are valid for answer ...
				    rr->resrec.rdnamehash == rr2->resrec.namehash &&
				    SameDomainName(&rr->resrec.rdata->u.srv.target, &rr2->resrec.name))		// ... whose name is the name of the SRV target
					AddRecordToResponseList(&nrp, rr2, rr);
	}

	// ***
	// *** 4. Parse Answer Section and cancel any records disallowed by Known-Answer list
	// ***
	for (i=0; i<query->h.numAnswers; i++)						// For each record in the query's answer section...
	{
		// Get the record...
		LargeCacheRecord *pkt;
		AuthRecord *rr;
		CacheRecord *ourcacherr;
		pkt = tls_mem_alloc(sizeof(LargeCacheRecord));
		if (!pkt) goto exit;
		memset(pkt, 0, sizeof(LargeCacheRecord));
		ptr = GetLargeResourceRecord(m, query, ptr, end, InterfaceID, kDNSRecordTypePacketAns, pkt);
		if (!ptr) { tls_mem_free(pkt); goto exit; }

		// See if this Known-Answer suppresses any of our currently planned answers
		for (rr=ResponseRecords; rr; rr=rr->NextResponse)
			if (MustSendRecord(rr) && ShouldSuppressKnownAnswer(&pkt->r, rr))
			{ rr->NR_AnswerTo = mDNSNULL; rr->NR_AdditionalTo = mDNSNULL; }

		// See if this Known-Answer suppresses any previously scheduled answers (for multi-packet KA suppression)
		for (rr=m->ResourceRecords; rr; rr=rr->next)
		{
			// If we're planning to send this answer on this interface, and only on this interface, then allow KA suppression
			if (rr->ImmedAnswer == InterfaceID && ShouldSuppressKnownAnswer(&pkt->r, rr))
			{
				if (srcaddr->type == mDNSAddrType_IPv4)
				{
					if (mDNSSameIPv4Address(rr->v4Requester, srcaddr->ip.v4)) rr->v4Requester = zeroIPAddr;
				}
				else if (srcaddr->type == mDNSAddrType_IPv6)
				{
					if (mDNSSameIPv6Address(rr->v6Requester, srcaddr->ip.v6)) rr->v6Requester = zerov6Addr;
				}
				if (mDNSIPv4AddressIsZero(rr->v4Requester) && mDNSIPv6AddressIsZero(rr->v6Requester)) rr->ImmedAnswer = mDNSNULL;
			}
		}

		// See if this Known-Answer suppresses any answers we were expecting for our cache records. We do this always,
		// even if the TC bit is not set (the TC bit will *not* be set in the *last* packet of a multi-packet KA list).
		ourcacherr = FindIdenticalRecordInCache(m, &pkt->r.resrec);
		if (ourcacherr && ourcacherr->MPExpectingKA && m->timenow - ourcacherr->MPLastUnansweredQT < mDNSPlatformOneSecond)
		{
			ourcacherr->MPUnansweredKA++;
			ourcacherr->MPExpectingKA = mDNSfalse;
		}

		// Having built our ExpectedAnswers list from the questions in this packet, we can definitively
		// remove from our ExpectedAnswers list any records that are suppressed in the very same packet.
		// For answers that are suppressed in subsequent KA list packets, we rely on the MPQ/MPKA counting to track them.
		eap = &ExpectedAnswers;
		while (*eap)
		{
			CacheRecord *rr = *eap;
			if (rr->resrec.InterfaceID == InterfaceID && IdenticalResourceRecord(&pkt->r.resrec, &rr->resrec))
			{ *eap = rr->NextInKAList; rr->NextInKAList = mDNSNULL; }
			else eap = &rr->NextInKAList;
		}
		
		// See if this Known-Answer is a surprise to us. If so, we shouldn't suppress our own query.
		if (!ourcacherr)
		{
			dqp = &DupQuestions;
			while (*dqp)
			{
				DNSQuestion *q = *dqp;
				if (ResourceRecordAnswersQuestion(&pkt->r.resrec, q))
				{ *dqp = q->NextInDQList; q->NextInDQList = mDNSNULL; }
				else dqp = &q->NextInDQList;
			}
		}
		tls_mem_free(pkt);
	}

	// ***
	// *** 5. Cancel any additionals that were added because of now-deleted records
	// ***
	for (rr=ResponseRecords; rr; rr=rr->NextResponse)
		if (rr->NR_AdditionalTo && !MustSendRecord(rr->NR_AdditionalTo))
		{ rr->NR_AnswerTo = mDNSNULL; rr->NR_AdditionalTo = mDNSNULL; }

	// ***
	// *** 6. Mark the send flags on the records we plan to send
	// ***
	for (rr=ResponseRecords; rr; rr=rr->NextResponse)
	{
		if (rr->NR_AnswerTo)
		{
			mDNSBool SendMulticastResponse = mDNSfalse;
			
			// If it's been a while since we multicast this, then send a multicast response for conflict detection, etc.
			if (m->timenow - (rr->LastMCTime + TicksTTL(rr)/4) >= 0) SendMulticastResponse = mDNStrue;
			
			// If the client insists on a multicast response, then we'd better send one
			if (rr->NR_AnswerTo == (mDNSu8*)~0) SendMulticastResponse = mDNStrue;
			else if (rr->NR_AnswerTo) HaveUnicastAnswer = mDNStrue;
	
			if (SendMulticastResponse)
			{
				// If we're already planning to send this on another interface, just send it on all interfaces
				if (rr->ImmedAnswer && rr->ImmedAnswer != InterfaceID)
				{
					rr->ImmedAnswer = mDNSInterfaceMark;
					m->NextScheduledResponse = m->timenow;
					debugf("ProcessQuery: %##s (%s) : Will send on all interfaces", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
				}
				else
				{
					rr->ImmedAnswer = InterfaceID;			// Record interface to send it on
					m->NextScheduledResponse = m->timenow;
					if (srcaddr->type == mDNSAddrType_IPv4)
					{
						if      (mDNSIPv4AddressIsZero(rr->v4Requester))                rr->v4Requester = srcaddr->ip.v4;
						else if (!mDNSSameIPv4Address(rr->v4Requester, srcaddr->ip.v4)) rr->v4Requester = onesIPv4Addr;
					}
					else if (srcaddr->type == mDNSAddrType_IPv6)
					{
						if      (mDNSIPv6AddressIsZero(rr->v6Requester))                rr->v6Requester = srcaddr->ip.v6;
						else if (!mDNSSameIPv6Address(rr->v6Requester, srcaddr->ip.v6)) rr->v6Requester = onesIPv6Addr;
					}
				}
			}
			if (rr->resrec.RecordType == kDNSRecordTypeShared)
			{
				if (query->h.flags.b[0] & kDNSFlag0_TC) delayresponse = mDNSPlatformOneSecond * 20;	// Divided by 50 = 400ms
				else                                    delayresponse = mDNSPlatformOneSecond;		// Divided by 50 = 20ms
			}
		}
		else if (rr->NR_AdditionalTo && rr->NR_AdditionalTo->NR_AnswerTo == (mDNSu8*)~0)
		{
			// Since additional records are an optimization anyway, we only ever send them on one interface at a time
			// If two clients on different interfaces do queries that invoke the same optional additional answer,
			// then the earlier client is out of luck
			rr->ImmedAdditional = InterfaceID;
			// No need to set m->NextScheduledResponse here
			// We'll send these additional records when we send them, or not, as the case may be
		}
	}

	// ***
	// *** 7. If we think other machines are likely to answer these questions, set our packet suppression timer
	// ***
	if (delayresponse && (!m->SuppressSending || (m->SuppressSending - m->timenow) < (delayresponse + 49) / 50))
	{
		// Pick a random delay:
		// We start with the base delay chosen above (typically either 1 second or 20 seconds),
		// and add a random value in the range 0-5 seconds (making 1-6 seconds or 20-25 seconds).
		// This is an integer value, with resolution determined by the platform clock rate.
		// We then divide that by 50 to get the delay value in ticks. We defer the division until last
		// to get better results on platforms with coarse clock granularity (e.g. ten ticks per second).
		// The +49 before dividing is to ensure we round up, not down, to ensure that even
		// on platforms where the native clock rate is less than fifty ticks per second,
		// we still guarantee that the final calculated delay is at least one platform tick.
		// We want to make sure we don't ever allow the delay to be zero ticks,
		// because if that happens we'll fail the Rendezvous Conformance Test.
		// Our final computed delay is 20-120ms for normal delayed replies,
		// or 400-500ms in the case of multi-packet known-answer lists.
		m->SuppressSending = m->timenow + (delayresponse + (mDNSs32)mDNSRandom((mDNSu32)mDNSPlatformOneSecond*5) + 49) / 50;
		if (m->SuppressSending == 0) m->SuppressSending = 1;
	}

	// ***
	// *** 8. If query is from a legacy client, generate a unicast response too
	// ***
	if (HaveUnicastAnswer)
		responseptr = GenerateUnicastResponse(query, end, InterfaceID, LegacyQuery, response, ResponseRecords);

 exit:
	// ***
	// *** 9. Finally, clear our link chains ready for use next time
	// ***
	while (ResponseRecords)
	{
		rr = ResponseRecords;
		ResponseRecords = rr->NextResponse;
		rr->NextResponse    = mDNSNULL;
		rr->NR_AnswerTo     = mDNSNULL;
		rr->NR_AdditionalTo = mDNSNULL;
	}
	
	while (ExpectedAnswers)
	{
		CacheRecord *rr;
		rr = ExpectedAnswers;
		ExpectedAnswers = rr->NextInKAList;
		rr->NextInKAList = mDNSNULL;
		
		// For non-truncated queries, we can definitively say that we should expect
		// to be seeing a response for any records still left in the ExpectedAnswers list
		if (!(query->h.flags.b[0] & kDNSFlag0_TC))
			if (rr->UnansweredQueries == 0 || m->timenow - rr->LastUnansweredTime >= mDNSPlatformOneSecond)
			{
				rr->UnansweredQueries++;
				rr->LastUnansweredTime = m->timenow;
				if (rr->UnansweredQueries > 1)
					debugf("ProcessQuery: (!TC) UAQ %lu MPQ %lu MPKA %lu %s",
					       rr->UnansweredQueries, rr->MPUnansweredQ, rr->MPUnansweredKA, GetRRDisplayString(m, rr));
				SetNextCacheCheckTime(m, rr);
			}

		// If we've seen multiple unanswered queries for this record,
		// then mark it to expire in five seconds if we don't get a response by then.
		if (rr->UnansweredQueries >= MaxUnansweredQueries)
		{
			// Only show debugging message if this record was not about to expire anyway
			if (RRExpireTime(rr) - m->timenow > 4 * mDNSPlatformOneSecond)
				debugf("ProcessQuery: (Max) UAQ %lu MPQ %lu MPKA %lu mDNS_Reconfirm() for %s",
				       rr->UnansweredQueries, rr->MPUnansweredQ, rr->MPUnansweredKA, GetRRDisplayString(m, rr));
			mDNS_Reconfirm_internal(m, rr, kDefaultReconfirmTimeForNoAnswer);
		}
		// Make a guess, based on the multi-packet query / known answer counts, whether we think we
		// should have seen an answer for this. (We multiply MPQ by 4 and MPKA by 5, to allow for
		// possible packet loss of up to 20% of the additional KA packets.)
		else if (rr->MPUnansweredQ * 4 > rr->MPUnansweredKA * 5 + 8)
		{
			// We want to do this conservatively.
			// If there are so many machines on the network that they have to use multi-packet known-answer lists,
			// then we don't want them to all hit the network simultaneously with their final expiration queries.
			// By setting the record to expire in four minutes, we achieve two things:
			// (a) the 90-95% final expiration queries will be less bunched together
			// (b) we allow some time for us to witness enough other failed queries that we don't have to do our own
			mDNSu32 remain = (mDNSu32)(RRExpireTime(rr) - m->timenow) / 4;
			if (remain > 240 * (mDNSu32)mDNSPlatformOneSecond)
				remain = 240 * (mDNSu32)mDNSPlatformOneSecond;
			
			// Only show debugging message if this record was not about to expire anyway
			if (RRExpireTime(rr) - m->timenow > 4 * mDNSPlatformOneSecond)
				debugf("ProcessQuery: (MPQ) UAQ %lu MPQ %lu MPKA %lu mDNS_Reconfirm() for %s",
				       rr->UnansweredQueries, rr->MPUnansweredQ, rr->MPUnansweredKA, GetRRDisplayString(m, rr));

			if (remain <= 60 * (mDNSu32)mDNSPlatformOneSecond)
				rr->UnansweredQueries++;	// Treat this as equivalent to one definite unanswered query
			rr->MPUnansweredQ  = 0;			// Clear MPQ/MPKA statistics
			rr->MPUnansweredKA = 0;
			rr->MPExpectingKA  = mDNSfalse;
			
			if (remain < kDefaultReconfirmTimeForNoAnswer)
				remain = kDefaultReconfirmTimeForNoAnswer;
			mDNS_Reconfirm_internal(m, rr, remain);
		}
	}
	
	while (DupQuestions)
	{
#if MDNS_DEBUGMSGS
		int i;
#endif
		DNSQuestion *q = DupQuestions;
		DupQuestions = q->NextInDQList;
		q->NextInDQList = mDNSNULL;
		i = RecordDupSuppressInfo(q->DupSuppress, m->timenow, InterfaceID, srcaddr->type);
		debugf("ProcessQuery: Recorded DSI for %##s (%s) on %p/%s %d", q->qname.c, DNSTypeName(q->qtype), InterfaceID,
		       srcaddr->type == mDNSAddrType_IPv4 ? "v4" : "v6", i);
	}

	return(responseptr);
}

mDNSlocal void mDNSCoreReceiveQuery(mDNS *const m, const DNSMessage *const msg, const mDNSu8 *const end,
				    const mDNSAddr *srcaddr, const mDNSIPPort srcport, const mDNSAddr *dstaddr, mDNSIPPort dstport,
				    const mDNSInterfaceID InterfaceID)
{
	DNSMessage    *response;
	const mDNSu8 *responseend    = mDNSNULL;
	
	verbosedebugf("Received Query from %#-15a:%d to %#-15a:%d on 0x%.8X with %2d Question%s %2d Answer%s %2d Authorit%s %2d Additional%s",
		      srcaddr, (mDNSu16)srcport.b[0]<<8 | srcport.b[1],
		      dstaddr, (mDNSu16)dstport.b[0]<<8 | dstport.b[1],
		      InterfaceID,
		      msg->h.numQuestions,   msg->h.numQuestions   == 1 ? ", " : "s,",
		      msg->h.numAnswers,     msg->h.numAnswers     == 1 ? ", " : "s,",
		      msg->h.numAuthorities, msg->h.numAuthorities == 1 ? "y,  " : "ies,",
		      msg->h.numAdditionals, msg->h.numAdditionals == 1 ? "" : "s");

	response = tls_mem_alloc(sizeof(DNSMessage));
	if(!response) return;
	responseend = ProcessQuery(m, msg, end, srcaddr, InterfaceID,
				   (srcport.NotAnInteger != MulticastDNSPort.NotAnInteger), mDNSAddrIsDNSMulticast(dstaddr), response);

	if (responseend)	// If responseend is non-null, that means we built a unicast response packet
	{
		debugf("Unicast Response: %d Question%s, %d Answer%s, %d Additional%s to %#-15a:%d on %p/%ld",
		       response->h.numQuestions,   response->h.numQuestions   == 1 ? "" : "s",
		       response->h.numAnswers,     response->h.numAnswers     == 1 ? "" : "s",
		       response->h.numAdditionals, response->h.numAdditionals == 1 ? "" : "s",
		       srcaddr, (mDNSu16)srcport.b[0]<<8 | srcport.b[1], InterfaceID, srcaddr->type);
		mDNSSendDNSMessage(m, response, responseend, InterfaceID, dstport, srcaddr, srcport);
	}
	tls_mem_free(response);
}

// NOTE: mDNSCoreReceiveResponse calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSlocal void mDNSCoreReceiveResponse(mDNS *const m,
				       const DNSMessage *const response, const mDNSu8 *end, const mDNSAddr *srcaddr, const mDNSAddr *dstaddr,
				       const mDNSInterfaceID InterfaceID, mDNSu8 ttl)
{
	int i;
	const mDNSu8 *ptr = LocateAnswers(response, end);	// We ignore questions (if any) in a DNS response packet
	CacheRecord *CacheFlushRecords = mDNSNULL;
	CacheRecord **cfp = &CacheFlushRecords;
		
	// All records in a DNS response packet are treated as equally valid statements of truth. If we want
	// to guard against spoof responses, then the only credible protection against that is cryptographic
	// security, e.g. DNSSEC., not worring about which section in the spoof packet contained the record
	int totalrecords = response->h.numAnswers + response->h.numAuthorities + response->h.numAdditionals;

	(void)srcaddr;	// Currently used only for display in debugging message

	verbosedebugf("Received Response from %#-15a addressed to %#-15a on %p TTL %d with %2d Question%s %2d Answer%s %2d Authorit%s %2d Additional%s",
		      srcaddr, dstaddr, InterfaceID, ttl,
		      response->h.numQuestions,   response->h.numQuestions   == 1 ? ", " : "s,",
		      response->h.numAnswers,     response->h.numAnswers     == 1 ? ", " : "s,",
		      response->h.numAuthorities, response->h.numAuthorities == 1 ? "y,  " : "ies,",
		      response->h.numAdditionals, response->h.numAdditionals == 1 ? "" : "s");

	// TTL should be 255
	// In the case of overlayed subnets that aren't using RFC 3442, some packets may incorrectly
	// go to the router first and then come back with a TTL of 254, so we allow that too.
	// Anything lower than 254 is a pretty good sign of an off-net spoofing attack.
	// Also, if we get a unicast response when we weren't expecting one, then we assume it is someone trying to spoof us
	if (ttl < 254 || (!mDNSAddrIsDNSMulticast(dstaddr) && (mDNSu32)(m->timenow - m->ExpectUnicastResponse) > (mDNSu32)mDNSPlatformOneSecond))
	{
		debugf("** Ignored apparent spoof mDNS Response from %#-15a to %#-15a TTL %d on %p with %2d Question%s %2d Answer%s %2d Authorit%s %2d Additional%s",
		       srcaddr, dstaddr, ttl, InterfaceID,
		       response->h.numQuestions,   response->h.numQuestions   == 1 ? ", " : "s,",
		       response->h.numAnswers,     response->h.numAnswers     == 1 ? ", " : "s,",
		       response->h.numAuthorities, response->h.numAuthorities == 1 ? "y,  " : "ies,",
		       response->h.numAdditionals, response->h.numAdditionals == 1 ? "" : "s");
		return;
	}

	for (i = 0; i < totalrecords && ptr && ptr < end; i++)
	{
		LargeCacheRecord *pkt = tls_mem_alloc(sizeof(LargeCacheRecord));
		const mDNSu8 RecordType = (mDNSu8)((i < response->h.numAnswers) ? kDNSRecordTypePacketAns : kDNSRecordTypePacketAdd);
		if (!pkt) break;
		memset(pkt, 0, sizeof(LargeCacheRecord));
		ptr = GetLargeResourceRecord(m, response, ptr, end, InterfaceID, RecordType, pkt);
		if (!ptr) { tls_mem_free(pkt); break; }		// Break out of the loop and clean up our CacheFlushRecords list before exiting

		// 1. Check that this packet resource record does not conflict with any of ours
		if (m->CurrentRecord) LogMsg("mDNSCoreReceiveResponse ERROR m->CurrentRecord already set");
		m->CurrentRecord = m->ResourceRecords;
		while (m->CurrentRecord)
		{
			AuthRecord *rr = m->CurrentRecord;
			m->CurrentRecord = rr->next;
			if (PacketRRMatchesSignature(&pkt->r, rr))		// If interface, name, type (if verified) and class match...
			{
				// ... check to see if rdata is identical
				if (SameRData(&pkt->r.resrec, &rr->resrec))
				{
					// If the RR in the packet is identical to ours, just check they're not trying to lower the TTL on us
					if (pkt->r.resrec.rroriginalttl >= rr->resrec.rroriginalttl/2 || m->SleepState)
					{
						// If we were planning to send on this -- and only this -- interface, then we don't need to any more
						if (rr->ImmedAnswer == InterfaceID) rr->ImmedAnswer = mDNSNULL;
					}
					else
					{
						if      (rr->ImmedAnswer == mDNSNULL)    { rr->ImmedAnswer = InterfaceID;       m->NextScheduledResponse = m->timenow; }
						else if (rr->ImmedAnswer != InterfaceID) { rr->ImmedAnswer = mDNSInterfaceMark; m->NextScheduledResponse = m->timenow; }
					}
				}
				else
				{
					// else, the packet RR has different rdata -- check to see if this is a conflict
					if (pkt->r.resrec.rroriginalttl > 0 && PacketRRConflict(m, rr, &pkt->r))
					{
						debugf("mDNSCoreReceiveResponse: Our Record: %08X %08X %s", rr->  resrec.rdatahash, rr->  resrec.rdnamehash, GetRRDisplayString(m, rr));
						debugf("mDNSCoreReceiveResponse: Pkt Record: %08X %08X %s", pkt->r.resrec.rdatahash, pkt->r.resrec.rdnamehash, GetRRDisplayString(m, &pkt->r));

						// If this record is marked DependentOn another record for conflict detection purposes,
						// then *that* record has to be bumped back to probing state to resolve the conflict
						while (rr->DependentOn) rr = rr->DependentOn;

						// If we've just whacked this record's ProbeCount, don't need to do it again
						if (rr->ProbeCount <= DefaultProbeCountForTypeUnique)
						{
							// If we'd previously verified this record, put it back to probing state and try again
							if (rr->resrec.RecordType == kDNSRecordTypeVerified)
							{
								debugf("mDNSCoreReceiveResponse: Reseting to Probing: %##s (%s)", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
								rr->resrec.RecordType     = kDNSRecordTypeUnique;
								rr->ProbeCount     = DefaultProbeCountForTypeUnique + 1;
								rr->ThisAPInterval = DefaultAPIntervalForRecordType(kDNSRecordTypeUnique);
								InitializeLastAPTime(m, rr);
								RecordProbeFailure(m, rr);	// Repeated late conflicts also cause us to back off to the slower probing rate
							}
							// If we're probing for this record, we just failed
							else if (rr->resrec.RecordType == kDNSRecordTypeUnique)
							{
								debugf("mDNSCoreReceiveResponse: Will rename %##s (%s)", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
								mDNS_Deregister_internal(m, rr, mDNS_Dereg_conflict);
							}
							// We assumed this record must be unique, but we were wrong.
							// (e.g. There are two mDNSResponders on the same machine giving
							// different answers for the reverse mapping record.)
							// This is simply a misconfiguration, and we don't try to recover from it.
							else if (rr->resrec.RecordType == kDNSRecordTypeKnownUnique)
							{
								debugf("mDNSCoreReceiveResponse: Unexpected conflict on %##s (%s) -- discarding our record",
								       rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
								mDNS_Deregister_internal(m, rr, mDNS_Dereg_conflict);
							}
							else
								debugf("mDNSCoreReceiveResponse: Unexpected record type %X %##s (%s)",
								       rr->resrec.RecordType, rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype));
						}
					}
					// Else, matching signature, different rdata, but not a considered a conflict.
					// If the packet record has the cache-flush bit set, then we check to see if we have to re-assert our record(s)
					// to rescue them (see note about "multi-homing and bridged networks" at the end of this function).
					else if ((pkt->r.resrec.RecordType & kDNSRecordTypePacketUniqueMask) && m->timenow - rr->LastMCTime > mDNSPlatformOneSecond/2)
					{ rr->ImmedAnswer = mDNSInterfaceMark; m->NextScheduledResponse = m->timenow; }
				}
			}
		}

		// 2. See if we want to add this packet resource record to our cache
		if (m->rrcache_size)	// Only try to cache answers if we have a cache to put them in
		{
			mDNSu32 slot = HashSlot(&pkt->r.resrec.name);
			CacheRecord *rr;
			// 2a. Check if this packet resource record is already in our cache
			for (rr = m->rrcache_hash[slot]; rr; rr=rr->next)
			{
				// If we found this exact resource record, refresh its TTL
				if (rr->resrec.InterfaceID == InterfaceID && IdenticalResourceRecord(&pkt->r.resrec, &rr->resrec))
				{
					if (pkt->r.resrec.rdlength > InlineCacheRDSize)
						verbosedebugf("Found record size %5d interface %p already in cache: %s",
							      pkt->r.resrec.rdlength, InterfaceID, GetRRDisplayString(m, &pkt->r));
					rr->TimeRcvd  = m->timenow;
					
					if (pkt->r.resrec.RecordType & kDNSRecordTypePacketUniqueMask)
					{
						// If this packet record has the kDNSClass_UniqueRRSet flag set, then add it to our cache flushing list
						if (rr->NextInCFList == mDNSNULL && cfp != &rr->NextInCFList)
						{ *cfp = rr; cfp = &rr->NextInCFList; }

						// If this packet record is marked unique, and our previous cached copy was not, then fix it
						if (!(rr->resrec.RecordType & kDNSRecordTypePacketUniqueMask))
						{
							DNSQuestion *q;
							for (q = m->Questions; q; q=q->next) if (ResourceRecordAnswersQuestion(&rr->resrec, q)) q->UniqueAnswers++;
							rr->resrec.RecordType = pkt->r.resrec.RecordType;
						}
					}

					if (pkt->r.resrec.rroriginalttl > 0)
					{
						rr->resrec.rroriginalttl = pkt->r.resrec.rroriginalttl;
						rr->UnansweredQueries = 0;
						rr->MPUnansweredQ     = 0;
						rr->MPUnansweredKA    = 0;
						rr->MPExpectingKA     = mDNSfalse;
					}
					else
					{
						// If the packet TTL is zero, that means we're deleting this record.
						// To give other hosts on the network a chance to protest, we push the deletion
						// out one second into the future. Also, we set UnansweredQueries to MaxUnansweredQueries.
						// Otherwise, we'll do final queries for this record at 80% and 90% of its apparent
						// lifetime (800ms and 900ms from now) which is a pointless waste of network bandwidth.
						rr->resrec.rroriginalttl = 1;
						rr->UnansweredQueries = MaxUnansweredQueries;
					}
					SetNextCacheCheckTime(m, rr);
					break;
				}
			}

			// If packet resource record not in our cache, add it now
			// (unless it is just a deletion of a record we never had, in which case we don't care)
			if (!rr && pkt->r.resrec.rroriginalttl > 0)
			{
				rr = GetFreeCacheRR(m, pkt->r.resrec.rdlength);
				if (!rr) debugf("No cache space to add record for %#s", pkt->r.resrec.name.c);
				else
				{
					RData *saveptr = rr->resrec.rdata;		// Save the rr->resrec.rdata pointer
					*rr = pkt->r;
					rr->resrec.rdata = saveptr;			// and then restore it after the structure assignment
					
					if (rr->resrec.RecordType & kDNSRecordTypePacketUniqueMask)
					{ *cfp = rr; cfp = &rr->NextInCFList; }
					// If this is an oversized record with external storage allocated, copy rdata to external storage
					if (pkt->r.resrec.rdlength > InlineCacheRDSize)
						mDNSPlatformMemCopy(pkt->r.resrec.rdata, rr->resrec.rdata, sizeofRDataHeader + pkt->r.resrec.rdlength);
					rr->next = mDNSNULL;					// Clear 'next' pointer
					*(m->rrcache_tail[slot]) = rr;			// Append this record to tail of cache slot list
					m->rrcache_tail[slot] = &(rr->next);	// Advance tail pointer
					m->rrcache_used[slot]++;
					//debugf("Adding RR %##s to cache (%d)", pkt.r.name.c, m->rrcache_used);
					memcpy(&rr->resrec.addr,srcaddr,sizeof(mDNSAddr)); // Shiro, I want to keep sender IP
					CacheRecordAdd(m, rr);
					// MUST do this AFTER CacheRecordAdd(), because that's what sets CRActiveQuestion for us
					SetNextCacheCheckTime(m, rr);
				}
			}
		}
		tls_mem_free(pkt);
	}

	// If we've just received one or more records with their cache flush bits set,
	// then scan that cache slot to see if there are any old stale records we need to flush
	while (CacheFlushRecords)
	{
		CacheRecord *r1 = CacheFlushRecords, *r2;
		CacheFlushRecords = CacheFlushRecords->NextInCFList;
		r1->NextInCFList = mDNSNULL;
		for (r2 = m->rrcache_hash[HashSlot(&r1->resrec.name)]; r2; r2=r2->next)
			if (SameResourceRecordSignature(&r1->resrec, &r2->resrec) && m->timenow - r2->TimeRcvd > mDNSPlatformOneSecond)
			{
				verbosedebugf("Cache flush %p X %p %##s (%s)", r1, r2, r2->resrec.name.c, DNSTypeName(r2->resrec.rrtype));
				// We set stale records to expire in one second.
				// This gives the owner a chance to rescue it if necessary.
				// This is important in the case of multi-homing and bridged networks:
				//   Suppose host X is on Ethernet. X then connects to an AirPort base station, which happens to be
				//   bridged onto the same Ethernet. When X announces its AirPort IP address with the cache-flush bit
				//   set, the AirPort packet will be bridged onto the Ethernet, and all other hosts on the Ethernet
				//   will promptly delete their cached copies of the (still valid) Ethernet IP address record.
				//   By delaying the deletion by one second, we give X a change to notice that this bridging has
				//   happened, and re-announce its Ethernet IP address to rescue it from deletion from all our caches.
				// We set UnansweredQueries to MaxUnansweredQueries to avoid expensive and unnecessary
				// final expiration queries for this record.
				r2->resrec.rroriginalttl = 1;
				r2->TimeRcvd          = m->timenow;
				r2->UnansweredQueries = MaxUnansweredQueries;
				SetNextCacheCheckTime(m, r2);
			}
	}
}

mDNSexport void mDNSCoreReceive(mDNS *const m, DNSMessage *const msg, const mDNSu8 *const end,
				const mDNSAddr *const srcaddr, const mDNSIPPort srcport, const mDNSAddr *const dstaddr, const mDNSIPPort dstport,
				const mDNSInterfaceID InterfaceID, mDNSu8 ttl)
{
	const mDNSu8 StdQ  = kDNSFlag0_QR_Query    | kDNSFlag0_OP_StdQuery;
	const mDNSu8 StdR  = kDNSFlag0_QR_Response | kDNSFlag0_OP_StdQuery;
	const mDNSu8 QR_OP = (mDNSu8)(msg->h.flags.b[0] & kDNSFlag0_QROP_Mask);
	
	// Read the integer parts which are in IETF byte-order (MSB first, LSB second)
	mDNSu8 *ptr = (mDNSu8 *)&msg->h.numQuestions;
	msg->h.numQuestions   = (mDNSu16)((mDNSu16)ptr[0] <<  8 | ptr[1]);
	msg->h.numAnswers     = (mDNSu16)((mDNSu16)ptr[2] <<  8 | ptr[3]);
	msg->h.numAuthorities = (mDNSu16)((mDNSu16)ptr[4] <<  8 | ptr[5]);
	msg->h.numAdditionals = (mDNSu16)((mDNSu16)ptr[6] <<  8 | ptr[7]);
	
	if (!m) { LogMsg("mDNSCoreReceive ERROR m is NULL"); return; }
	
	// We use zero addresses and all-ones addresses at various places in the code to indicate special values like "no address"
	// If we accept and try to process a packet with zero or all-ones source address, that could really mess things up
	if (!mDNSAddressIsValid(srcaddr)) { debugf("mDNSCoreReceive ignoring packet from %#a", srcaddr); return; }
	
	mDNS_Lock(m);
	if      (QR_OP == StdQ) mDNSCoreReceiveQuery   (m, msg, end, srcaddr, srcport, dstaddr, dstport, InterfaceID);
	else if (QR_OP == StdR) mDNSCoreReceiveResponse(m, msg, end, srcaddr,          dstaddr,          InterfaceID, ttl);
	else debugf("Unknown DNS packet type %02X%02X (ignored)", msg->h.flags.b[0], msg->h.flags.b[1]);

	// Packet reception often causes a change to the task list:
	// 1. Inbound queries can cause us to need to send responses
	// 2. Conflicing response packets received from other hosts can cause us to need to send defensive responses
	// 3. Other hosts announcing deletion of shared records can cause us to need to re-assert those records
	// 4. Response packets that answer questions may cause our client to issue new questions
	mDNS_Unlock(m);
}

// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark -
#pragma mark - Searcher Functions
#endif
#if 0
mDNSlocal DNSQuestion *FindDuplicateQuestion(const mDNS *const m, const DNSQuestion *const question)
{
	DNSQuestion *q;
	// Note: A question can only be marked as a duplicate of one that occurs *earlier* in the list.
	// This prevents circular references, where two questions are each marked as a duplicate of the other.
	// Accordingly, we break out of the loop when we get to 'question', because there's no point searching
	// further in the list.
	for (q = m->Questions; q && q != question; q=q->next)		// Scan our list of questions
		if (q->InterfaceID == question->InterfaceID &&			// for another question with the same InterfaceID,
		    q->qtype       == question->qtype       &&			// type,
		    q->qclass      == question->qclass      &&			// class,
		    q->qnamehash   == question->qnamehash   &&
		    SameDomainName(&q->qname, &question->qname))		// and name
			return(q);
	return(mDNSNULL);
}

// This is called after a question is deleted, in case other identical questions were being
// suppressed as duplicates
mDNSlocal void UpdateQuestionDuplicates(mDNS *const m, const DNSQuestion *const question)
{
	DNSQuestion *q;
	for (q = m->Questions; q; q=q->next)		// Scan our list of questions
		if (q->DuplicateOf == question)			// To see if any questions were referencing this as their duplicate
		{
			q->ThisQInterval = question->ThisQInterval;
			q->LastQTime     = question->LastQTime;
			q->RecentAnswers = 0;
			q->DuplicateOf   = FindDuplicateQuestion(m, q);
			q->LastQTxTime   = question->LastQTxTime;
			SetNextQueryTime(m,q);
		}
}

mDNSlocal mStatus mDNS_StartQuery_internal(mDNS *const m, DNSQuestion *const question)
{
#if TEST_LOCALONLY_FOR_EVERYTHING
	question->InterfaceID = (mDNSInterfaceID)~0;
#endif
	if (m->rrcache_size == 0)	// Can't do queries if we have no cache space allocated
		return(mStatus_NoCache);
	else
	{
		int i;
		// Note: It important that new questions are appended at the *end* of the list, not prepended at the start
		DNSQuestion **q = &m->Questions;
		if (question->InterfaceID == ((mDNSInterfaceID)~0)) q = &m->LocalOnlyQuestions;
		while (*q && *q != question) q=&(*q)->next;

		if (*q)
		{
			LogMsg("Error! Tried to add a question %##s (%s) that's already in the active list",
			       question->qname.c, DNSTypeName(question->qtype));
			return(mStatus_AlreadyRegistered);
		}

		// If this question is referencing a specific interface, make sure it exists
		if (question->InterfaceID && question->InterfaceID != ((mDNSInterfaceID)~0))
		{
			NetworkInterfaceInfo *intf;
			for (intf = m->HostInterfaces; intf; intf = intf->next)
				if (intf->InterfaceID == question->InterfaceID) break;
			if (!intf)
			{
				debugf("mDNS_StartQuery_internal: Question %##s InterfaceID %p not found", question->qname.c, question->InterfaceID);
				return(mStatus_BadReferenceErr);
			}
		}

		// Note: In the case where we already have the answer to this question in our cache, that may be all the client
		// wanted, and they may immediately cancel their question. In this case, sending an actual query on the wire would
		// be a waste. For that reason, we schedule our first query to go out in half a second. If AnswerNewQuestion() finds
		// that we have *no* relevant answers currently in our cache, then it will accelerate that to go out immediately.
		if (!ValidateDomainName(&question->qname))
		{
			LogMsg("Attempt to start query with invalid qname %##s %s", question->qname.c, DNSTypeName(question->qtype));
			return(mStatus_Invalid);
		}

		if (!m->RandomQueryDelay) m->RandomQueryDelay = 1 + (mDNSs32)mDNSRandom((mDNSu32)InitialQuestionInterval);

		question->next           = mDNSNULL;
		question->qnamehash      = DomainNameHashValue(&question->qname);	// MUST do this before FindDuplicateQuestion()
		question->ThisQInterval  = InitialQuestionInterval * 2;				// MUST be > zero for an active question
		question->LastQTime      = m->timenow - m->RandomQueryDelay;		// Avoid inter-machine synchronization
		question->RecentAnswers  = 0;
		question->CurrentAnswers = 0;
		question->LargeAnswers   = 0;
		question->UniqueAnswers  = 0;
		question->DuplicateOf    = FindDuplicateQuestion(m, question);
		question->NextInDQList   = mDNSNULL;
		for (i=0; i<DupSuppressInfoSize; i++)
			question->DupSuppress[i].InterfaceID = mDNSNULL;
		// question->InterfaceID must be already set by caller
		question->SendQNow       = mDNSNULL;
		question->SendOnAll      = mDNSfalse;
		question->LastQTxTime    = m->timenow;

		if (!question->DuplicateOf)
			verbosedebugf("mDNS_StartQuery_internal: Question %##s %s %p (%p) started",
				      question->qname.c, DNSTypeName(question->qtype), question->InterfaceID, question);
		else
			verbosedebugf("mDNS_StartQuery_internal: Question %##s %s %p (%p) duplicate of (%p)",
				      question->qname.c, DNSTypeName(question->qtype), question->InterfaceID, question, question->DuplicateOf);

		*q = question;
		if (question->InterfaceID == ((mDNSInterfaceID)~0))
		{
			if (!m->NewLocalOnlyQuestions) m->NewLocalOnlyQuestions = question;
		}
		else
		{
			if (!m->NewQuestions) m->NewQuestions = question;
			SetNextQueryTime(m,question);
		}

		return(mStatus_NoError);
	}
}

mDNSlocal mStatus mDNS_StopQuery_internal(mDNS *const m, DNSQuestion *const question)
{
	CacheRecord *rr;
	DNSQuestion **q = &m->Questions;
	if (question->InterfaceID == ((mDNSInterfaceID)~0)) q = &m->LocalOnlyQuestions;
	while (*q && *q != question) q=&(*q)->next;
	if (*q) *q = (*q)->next;
	else
	{
		if (question->ThisQInterval >= 0)	// Only log error message if the query was supposed to be active
			LogMsg("mDNS_StopQuery_internal: Question %##s (%s) not found in active list",
			       question->qname.c, DNSTypeName(question->qtype));
		return(mStatus_BadReferenceErr);
	}

	// Take care to cut question from list *before* calling UpdateQuestionDuplicates
	UpdateQuestionDuplicates(m, question);
	// But don't trash ThisQInterval until afterwards.
	question->ThisQInterval = -1;

	// If there are any cache records referencing this as their active question, then see if any other
	// question that is also referencing them, else their CRActiveQuestion needs to get set to NULL.
	for (rr = m->rrcache_hash[HashSlot(&question->qname)]; rr; rr=rr->next)
	{
		if (rr->CRActiveQuestion == question)
		{
			DNSQuestion *q;
			for (q = m->Questions; q; q=q->next)		// Scan our list of questions
				if (ActiveQuestion(q) && ResourceRecordAnswersQuestion(&rr->resrec, q))
					break;
			verbosedebugf("mDNS_StopQuery_internal: Cache RR %##s (%s) setting CRActiveQuestion to %X", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), q);
			rr->CRActiveQuestion = q;		// Question used to be active; new value may or may not be null
			if (!q) m->rrcache_active--;	// If no longer active, decrement rrcache_active count
		}
	}

	// If we just deleted the question that CacheRecordAdd() or CacheRecordRmv()is about to look at,
	// bump its pointer forward one question.
	if (m->CurrentQuestion == question)
	{
		debugf("mDNS_StopQuery_internal: Just deleted the currently active question: %##s (%s)",
		       question->qname.c, DNSTypeName(question->qtype));
		m->CurrentQuestion = question->next;
	}

	if (m->NewQuestions == question)
	{
		debugf("mDNS_StopQuery_internal: Just deleted a new question that wasn't even answered yet: %##s (%s)",
		       question->qname.c, DNSTypeName(question->qtype));
		m->NewQuestions = question->next;
	}

	if (m->NewLocalOnlyQuestions == question) m->NewLocalOnlyQuestions = question->next;

	// Take care not to trash question->next until *after* we've updated m->CurrentQuestion and m->NewQuestions
	question->next = mDNSNULL;
	return(mStatus_NoError);
}

mDNSexport mStatus mDNS_StartQuery(mDNS *const m, DNSQuestion *const question)
{
	mStatus status;
	mDNS_Lock(m);
	status = mDNS_StartQuery_internal(m, question);
	mDNS_Unlock(m);
	return(status);
}

mDNSexport mStatus mDNS_StopQuery(mDNS *const m, DNSQuestion *const question)
{
	mStatus status;
	mDNS_Lock(m);
	status = mDNS_StopQuery_internal(m, question);
	mDNS_Unlock(m);
	return(status);
}

mDNSexport mStatus mDNS_Reconfirm(mDNS *const m, CacheRecord *const rr)
{
	mStatus status;
	mDNS_Lock(m);
	status = mDNS_Reconfirm_internal(m, rr, kDefaultReconfirmTimeForNoAnswer);
	mDNS_Unlock(m);
	return(status);
}

mDNSexport mStatus mDNS_ReconfirmByValue(mDNS *const m, ResourceRecord *const rr)
{
	mStatus status = mStatus_BadReferenceErr;
	CacheRecord *cr;
	mDNS_Lock(m);
	cr = FindIdenticalRecordInCache(m, rr);
	if (cr) status = mDNS_Reconfirm_internal(m, cr, kDefaultReconfirmTimeForNoAnswer);
	mDNS_Unlock(m);
	return(status);
}

mDNSexport mStatus mDNS_StartBrowse(mDNS *const m, DNSQuestion *const question,
				    const domainname *const srv, const domainname *const domain,
				    const mDNSInterfaceID InterfaceID, mDNSQuestionCallback *Callback, void *Context)
{
	question->ThisQInterval     = -1;				// Indicate that query is not yet active
	question->InterfaceID       = InterfaceID;
	question->qtype             = kDNSType_PTR;
	question->qclass            = kDNSClass_IN;
	question->QuestionCallback  = Callback;
	question->QuestionContext   = Context;
	if (!ConstructServiceName(&question->qname, mDNSNULL, srv, domain)) return(mStatus_BadParamErr);
	return(mDNS_StartQuery(m, question));
}

mDNSlocal void FoundServiceInfoSRV(mDNS *const m, DNSQuestion *question, const ResourceRecord *const answer, mDNSBool AddRecord)
{
	ServiceInfoQuery *query = (ServiceInfoQuery *)question->QuestionContext;
	mDNSBool PortChanged = (mDNSBool)(query->info->port.NotAnInteger != answer->rdata->u.srv.port.NotAnInteger);
	if (!AddRecord) return;
	if (answer->rrtype != kDNSType_SRV) return;

	query->info->port = answer->rdata->u.srv.port;

	// If this is our first answer, then set the GotSRV flag and start the address query
	if (!query->GotSRV)
	{
		query->GotSRV             = mDNStrue;
		query->qAv4.InterfaceID   = answer->InterfaceID;
		AssignDomainName(query->qAv4.qname, answer->rdata->u.srv.target);
		query->qAv6.InterfaceID   = answer->InterfaceID;
		AssignDomainName(query->qAv6.qname, answer->rdata->u.srv.target);
		mDNS_StartQuery_internal(m, &query->qAv4);
		mDNS_StartQuery_internal(m, &query->qAv6);
	}
	// If this is not our first answer, only re-issue the address query if the target host name has changed
	else if ((query->qAv4.InterfaceID != query->qSRV.InterfaceID && query->qAv4.InterfaceID != answer->InterfaceID) ||
		 !SameDomainName(&query->qAv4.qname, &answer->rdata->u.srv.target))
	{
		mDNS_StopQuery_internal(m, &query->qAv4);
		mDNS_StopQuery_internal(m, &query->qAv6);
		if (SameDomainName(&query->qAv4.qname, &answer->rdata->u.srv.target) && !PortChanged)
		{
			// If we get here, it means:
			// 1. This is not our first SRV answer
			// 2. The interface ID is different, but the target host and port are the same
			// This implies that we're seeing the exact same SRV record on more than one interface, so we should
			// make our address queries at least as broad as the original SRV query so that we catch all the answers.
			query->qAv4.InterfaceID = query->qSRV.InterfaceID;	// Will be mDNSInterface_Any, or a specific interface
			query->qAv6.InterfaceID = query->qSRV.InterfaceID;
		}
		else
		{
			query->qAv4.InterfaceID   = answer->InterfaceID;
			AssignDomainName(query->qAv4.qname, answer->rdata->u.srv.target);
			query->qAv6.InterfaceID   = answer->InterfaceID;
			AssignDomainName(query->qAv6.qname, answer->rdata->u.srv.target);
		}
		debugf("FoundServiceInfoSRV: Restarting address queries for %##s", query->qAv4.qname.c);
		mDNS_StartQuery_internal(m, &query->qAv4);
		mDNS_StartQuery_internal(m, &query->qAv6);
	}
	else if (query->ServiceInfoQueryCallback && query->GotADD && query->GotTXT && PortChanged)
	{
		if (++query->Answers >= 100)
			debugf("**** WARNING **** Have given %lu answers for %##s (SRV) %##s %u",
			       query->Answers, query->qSRV.qname.c, answer->rdata->u.srv.target.c,
			       ((mDNSu16)answer->rdata->u.srv.port.b[0] << 8) | answer->rdata->u.srv.port.b[1]);
		query->ServiceInfoQueryCallback(m, query);
	}
	// CAUTION: MUST NOT do anything more with query after calling query->Callback(), because the client's
	// callback function is allowed to do anything, including deleting this query and freeing its memory.
}

mDNSlocal void FoundServiceInfoTXT(mDNS *const m, DNSQuestion *question, const ResourceRecord *const answer, mDNSBool AddRecord)
{
	ServiceInfoQuery *query = (ServiceInfoQuery *)question->QuestionContext;
	if (!AddRecord) return;
	if (answer->rrtype != kDNSType_TXT) return;
	if (answer->rdlength > sizeof(query->info->TXTinfo)) return;

	query->GotTXT       = mDNStrue;
	query->info->TXTlen = answer->rdlength;
	mDNSPlatformMemCopy(answer->rdata->u.txt.c, query->info->TXTinfo, answer->rdlength);

	verbosedebugf("FoundServiceInfoTXT: %##s GotADD=%d", query->info->name.c, query->GotADD);

	// CAUTION: MUST NOT do anything more with query after calling query->Callback(), because the client's
	// callback function is allowed to do anything, including deleting this query and freeing its memory.
	if (query->ServiceInfoQueryCallback && query->GotADD)
	{
		if (++query->Answers >= 100)
			debugf("**** WARNING **** have given %lu answers for %##s (TXT) %#s...",
			       query->Answers, query->qSRV.qname.c, answer->rdata->u.txt.c);
		query->ServiceInfoQueryCallback(m, query);
	}
}

mDNSlocal void FoundServiceInfo(mDNS *const m, DNSQuestion *question, const ResourceRecord *const answer, mDNSBool AddRecord)
{
	ServiceInfoQuery *query = (ServiceInfoQuery *)question->QuestionContext;
	if (!AddRecord) return;
	
	if (answer->rrtype == kDNSType_A)
	{
		query->info->ip.type = mDNSAddrType_IPv4;
		query->info->ip.ip.v4 = answer->rdata->u.ip;
	}
	else if (answer->rrtype == kDNSType_AAAA)
	{
		query->info->ip.type = mDNSAddrType_IPv6;
		query->info->ip.ip.v6 = answer->rdata->u.ipv6;
	}
	else
	{
		debugf("FoundServiceInfo: answer %##s type %d (%s) unexpected", answer->name.c, answer->rrtype, DNSTypeName(answer->rrtype));
		return;
	}

	query->GotADD = mDNStrue;
	query->info->InterfaceID = answer->InterfaceID;

	verbosedebugf("FoundServiceInfo v%d: %##s GotTXT=%d", query->info->ip.type, query->info->name.c, query->GotTXT);

	// CAUTION: MUST NOT do anything more with query after calling query->Callback(), because the client's
	// callback function is allowed to do anything, including deleting this query and freeing its memory.
	if (query->ServiceInfoQueryCallback && query->GotTXT)
	{
		if (++query->Answers >= 100)
		{
			if (answer->rrtype == kDNSType_A)
				debugf("**** WARNING **** have given %lu answers for %##s (A) %.4a",     query->Answers, query->qSRV.qname.c, &answer->rdata->u.ip);
			else
				debugf("**** WARNING **** have given %lu answers for %##s (AAAA) %.16a", query->Answers, query->qSRV.qname.c, &answer->rdata->u.ipv6);
		}
		query->ServiceInfoQueryCallback(m, query);
	}
}

// On entry, the client must have set the name and InterfaceID fields of the ServiceInfo structure
// If the query is not interface-specific, then InterfaceID may be zero
// Each time the Callback is invoked, the remainder of the fields will have been filled in
// In addition, InterfaceID will be updated to give the interface identifier corresponding to that response
mDNSexport mStatus mDNS_StartResolveService(mDNS *const m,
					    ServiceInfoQuery *query, ServiceInfo *info, mDNSServiceInfoQueryCallback *Callback, void *Context)
{
	mStatus status;
	mDNS_Lock(m);

	query->qSRV.ThisQInterval       = -1;		// This question not yet in the question list
	query->qSRV.InterfaceID         = info->InterfaceID;
	AssignDomainName(query->qSRV.qname, info->name);
	query->qSRV.qtype               = kDNSType_SRV;
	query->qSRV.qclass              = kDNSClass_IN;
	query->qSRV.QuestionCallback    = FoundServiceInfoSRV;
	query->qSRV.QuestionContext     = query;

	query->qTXT.ThisQInterval       = -1;		// This question not yet in the question list
	query->qTXT.InterfaceID         = info->InterfaceID;
	AssignDomainName(query->qTXT.qname, info->name);
	query->qTXT.qtype               = kDNSType_TXT;
	query->qTXT.qclass              = kDNSClass_IN;
	query->qTXT.QuestionCallback    = FoundServiceInfoTXT;
	query->qTXT.QuestionContext     = query;

	query->qAv4.ThisQInterval       = -1;		// This question not yet in the question list
	query->qAv4.InterfaceID         = info->InterfaceID;
	query->qAv4.qname.c[0]          = 0;
	query->qAv4.qtype               = kDNSType_A;
	query->qAv4.qclass              = kDNSClass_IN;
	query->qAv4.QuestionCallback    = FoundServiceInfo;
	query->qAv4.QuestionContext     = query;

	query->qAv6.ThisQInterval       = -1;		// This question not yet in the question list
	query->qAv6.InterfaceID         = info->InterfaceID;
	query->qAv6.qname.c[0]          = 0;
	query->qAv6.qtype               = kDNSType_AAAA;
	query->qAv6.qclass              = kDNSClass_IN;
	query->qAv6.QuestionCallback    = FoundServiceInfo;
	query->qAv6.QuestionContext     = query;

	query->GotSRV                   = mDNSfalse;
	query->GotTXT                   = mDNSfalse;
	query->GotADD                   = mDNSfalse;
	query->Answers                  = 0;

	query->info                     = info;
	query->ServiceInfoQueryCallback = Callback;
	query->ServiceInfoQueryContext  = Context;

//	info->name      = Must already be set up by client
//	info->interface = Must already be set up by client
	info->ip        = zeroAddr;
	info->port      = zeroIPPort;
	info->TXTlen    = 0;

	status = mDNS_StartQuery_internal(m, &query->qSRV);
	if (status == mStatus_NoError) status = mDNS_StartQuery_internal(m, &query->qTXT);
	if (status != mStatus_NoError) mDNS_StopResolveService(m, query);

	mDNS_Unlock(m);
	return(status);
}

mDNSexport void    mDNS_StopResolveService (mDNS *const m, ServiceInfoQuery *query)
{
	mDNS_Lock(m);
	if (query->qSRV.ThisQInterval >= 0) mDNS_StopQuery_internal(m, &query->qSRV);
	if (query->qTXT.ThisQInterval >= 0) mDNS_StopQuery_internal(m, &query->qTXT);
	if (query->qAv4.ThisQInterval >= 0) mDNS_StopQuery_internal(m, &query->qAv4);
	if (query->qAv6.ThisQInterval >= 0) mDNS_StopQuery_internal(m, &query->qAv6);
	mDNS_Unlock(m);
}

mDNSexport mStatus mDNS_GetDomains(mDNS *const m, DNSQuestion *const question, mDNS_DomainType DomainType,
				   const mDNSInterfaceID InterfaceID, mDNSQuestionCallback *Callback, void *Context)
{
	MakeDomainNameFromDNSNameString(&question->qname, mDNS_DomainTypeNames[DomainType]);
	question->InterfaceID      = InterfaceID;
	question->qtype            = kDNSType_PTR;
	question->qclass           = kDNSClass_IN;
	question->QuestionCallback = Callback;
	question->QuestionContext  = Context;

	// No sense doing this until we actually support unicast query/update
	//return(mDNS_StartQuery(m, question));
	(void)m; // Unused
	return(mStatus_NoError);
}
#endif
// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark - Responder Functions
#endif

// Set up a AuthRecord with sensible default values.
// These defaults may be overwritten with new values before mDNS_Register is called
mDNSexport void mDNS_SetupResourceRecord(AuthRecord *rr, RData *RDataStorage, mDNSInterfaceID InterfaceID,
					 mDNSu16 rrtype, mDNSu32 ttl, mDNSu8 RecordType, mDNSRecordCallback Callback, void *Context)
{
	// Don't try to store a TTL bigger than we can represent in platform time units
	if (ttl > 0x7FFFFFFFUL / mDNSPlatformOneSecond)
		ttl = 0x7FFFFFFFUL / mDNSPlatformOneSecond;
	else if (ttl == 0)		// And Zero TTL is illegal
		ttl = kDefaultTTLforShared;

	// Field Group 1: Persistent metadata for Authoritative Records
	rr->Additional1       = mDNSNULL;
	rr->Additional2       = mDNSNULL;
	rr->DependentOn       = mDNSNULL;
	rr->RRSet             = mDNSNULL;
	rr->RecordCallback    = Callback;
	rr->RecordContext     = Context;

	rr->resrec.RecordType        = RecordType;
	rr->HostTarget        = mDNSfalse;
	
	// Field Group 2: Transient state for Authoritative Records (set in mDNS_Register_internal)
	// Field Group 3: Transient state for Cache Records         (set in mDNS_Register_internal)

	// Field Group 4: The actual information pertaining to this resource record
	rr->resrec.InterfaceID       = InterfaceID;
	rr->resrec.name.c[0]         = 0;		// MUST be set by client
	rr->resrec.rrtype            = rrtype;
	rr->resrec.rrclass           = kDNSClass_IN;
	rr->resrec.rroriginalttl     = ttl;
//	rr->resrec.rdlength          = MUST set by client and/or in mDNS_Register_internal
//	rr->resrec.rdestimate        = set in mDNS_Register_internal
//	rr->resrec.rdata             = MUST be set by client

	if (RDataStorage)
		rr->resrec.rdata = RDataStorage;
	else
	{
		rr->resrec.rdata = &rr->rdatastorage;
		rr->resrec.rdata->MaxRDLength = sizeof(RDataBody);
	}
}

mDNSexport mStatus mDNS_Register(mDNS *const m, AuthRecord *const rr)
{
	mStatus status;
	mDNS_Lock(m);
	status = mDNS_Register_internal(m, rr);
	mDNS_Unlock(m);
	return(status);
}
#if 1
mDNSexport mStatus mDNS_Update(mDNS *const m, AuthRecord *const rr, mDNSu32 newttl,
			       const mDNSu16 newrdlength,
			       RData *const newrdata, mDNSRecordUpdateCallback *Callback)
{
	if (!ValidateRData(rr->resrec.rrtype, newrdlength, newrdata))
	{ LogMsg("Attempt to update record with invalid rdata: %s", GetRRDisplayString_rdb(m, &rr->resrec, &newrdata->u)); return(mStatus_Invalid); }

	mDNS_Lock(m);

	// If TTL is unspecified, leave TTL unchanged
	if (newttl == 0) newttl = rr->resrec.rroriginalttl;

	// If we already have an update queued up which has not gone through yet,
	// give the client a chance to free that memory
	if (rr->NewRData)
	{
		RData *n = rr->NewRData;
		rr->NewRData = mDNSNULL;			// Clear the NewRData pointer ...
		if (rr->UpdateCallback)
			rr->UpdateCallback(m, rr, n);	// ...and let the client free this memory, if necessary
	}
	
	if (rr->AnnounceCount < ReannounceCount)
		rr->AnnounceCount = ReannounceCount;
	rr->ThisAPInterval       = DefaultAPIntervalForRecordType(rr->resrec.RecordType);
	InitializeLastAPTime(m, rr);
	rr->NewRData             = newrdata;
	rr->newrdlength          = newrdlength;
	rr->UpdateCallback       = Callback;
	if (!rr->UpdateBlocked && rr->UpdateCredits) rr->UpdateCredits--;
	if (!rr->NextUpdateCredit) rr->NextUpdateCredit = (m->timenow + mDNSPlatformOneSecond * 60) | 1;
	if (rr->AnnounceCount > rr->UpdateCredits + 1) rr->AnnounceCount = (mDNSu8)(rr->UpdateCredits + 1);
	if (rr->UpdateCredits <= 5)
	{
		mDNSs32 delay = 1 << (5 - rr->UpdateCredits);
		if (!rr->UpdateBlocked) rr->UpdateBlocked = (m->timenow + delay * mDNSPlatformOneSecond) | 1;
		rr->LastAPTime = rr->UpdateBlocked;
		rr->ThisAPInterval *= 4;
		LogMsg("Excessive update rate for %##s; delaying announcement by %d seconds", rr->resrec.name.c, delay);
	}
	rr->resrec.rroriginalttl = newttl;
	mDNS_Unlock(m);
	return(mStatus_NoError);
}
#endif
// NOTE: mDNS_Deregister calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSexport mStatus mDNS_Deregister(mDNS *const m, AuthRecord *const rr)
{
	mStatus status;
	mDNS_Lock(m);
	status = mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
	mDNS_Unlock(m);
	return(status);
}

mDNSlocal void HostNameCallback(mDNS *const m, AuthRecord *const rr, mStatus result);

mDNSlocal NetworkInterfaceInfo *FindFirstAdvertisedInterface(mDNS *const m)
{
	NetworkInterfaceInfo *intf;
	for (intf = m->HostInterfaces; intf; intf = intf->next)
		if (intf->Advertise) break;
	return(intf);
}

mDNSlocal void mDNS_AdvertiseInterface(mDNS *const m, NetworkInterfaceInfo *set)
{
	char *buffer = tls_mem_alloc(256);
	NetworkInterfaceInfo *primary = FindFirstAdvertisedInterface(m);
	if (!primary) primary = set; // If no existing advertised interface, this new NetworkInterfaceInfo becomes our new primary
	if(!buffer) return ;
	
	mDNS_SetupResourceRecord(&set->RR_A,     mDNSNULL, set->InterfaceID, kDNSType_A,     kDefaultTTLforUnique, kDNSRecordTypeUnique,      HostNameCallback, set);
	mDNS_SetupResourceRecord(&set->RR_PTR,   mDNSNULL, set->InterfaceID, kDNSType_PTR,   kDefaultTTLforUnique, kDNSRecordTypeKnownUnique, mDNSNULL, mDNSNULL);
	mDNS_SetupResourceRecord(&set->RR_HINFO, mDNSNULL, set->InterfaceID, kDNSType_HINFO, kDefaultTTLforUnique, kDNSRecordTypeUnique,      mDNSNULL, mDNSNULL);

	// 1. Set up Address record to map from host name ("foo.local.") to IP address
	// 2. Set up reverse-lookup PTR record to map from our address back to our host name
	AssignDomainName(set->RR_A.resrec.name, m->hostname);
	if (set->ip.type == mDNSAddrType_IPv4)
	{
		set->RR_A.resrec.rrtype = kDNSType_A;
		set->RR_A.resrec.rdata->u.ip = set->ip.ip.v4;
		// Note: This is reverse order compared to a normal dotted-decimal IP address
		mDNS_snprintf(buffer, 256, "%d.%d.%d.%d.in-addr.arpa.",
			      set->ip.ip.v4.b[3], set->ip.ip.v4.b[2], set->ip.ip.v4.b[1], set->ip.ip.v4.b[0]);
	}
	else if (set->ip.type == mDNSAddrType_IPv6)
	{
		int i;
		set->RR_A.resrec.rrtype = kDNSType_AAAA;
		set->RR_A.resrec.rdata->u.ipv6 = set->ip.ip.v6;
		for (i = 0; i < 16; i++)
		{
			static const char hexValues[] = "0123456789ABCDEF";
			buffer[i * 4    ] = hexValues[set->ip.ip.v6.b[15 - i] & 0x0F];
			buffer[i * 4 + 1] = '.';
			buffer[i * 4 + 2] = hexValues[set->ip.ip.v6.b[15 - i] >> 4];
			buffer[i * 4 + 3] = '.';
		}
		mDNS_snprintf(&buffer[64], 256-64, "ip6.arpa.");
	}

	MakeDomainNameFromDNSNameString(&set->RR_PTR.resrec.name, buffer);
	tls_mem_free(buffer);
	set->RR_PTR.HostTarget = mDNStrue;	// Tell mDNS that the target of this PTR is to be kept in sync with our host name

	set->RR_A.RRSet = &primary->RR_A;	// May refer to self

	mDNS_Register_internal(m, &set->RR_A);
	mDNS_Register_internal(m, &set->RR_PTR);

	if (m->HIHardware.c[0] > 0 && m->HISoftware.c[0] > 0 && m->HIHardware.c[0] + m->HISoftware.c[0] <= 254)
	{
		mDNSu8 *p = set->RR_HINFO.resrec.rdata->u.data;
		AssignDomainName(set->RR_HINFO.resrec.name, m->hostname);
		set->RR_HINFO.DependentOn = &set->RR_A;
		mDNSPlatformMemCopy(&m->HIHardware, p, 1 + (mDNSu32)m->HIHardware.c[0]);
		p += 1 + (int)p[0];
		mDNSPlatformMemCopy(&m->HISoftware, p, 1 + (mDNSu32)m->HISoftware.c[0]);
		mDNS_Register_internal(m, &set->RR_HINFO);
	}
	else
	{
		debugf("Not creating HINFO record: platform support layer provided no information");
		set->RR_HINFO.resrec.RecordType = kDNSRecordTypeUnregistered;
	}
}

mDNSlocal void mDNS_DeadvertiseInterface(mDNS *const m, NetworkInterfaceInfo *set)
{
	NetworkInterfaceInfo *intf;
	// If we still have address records referring to this one, update them
	NetworkInterfaceInfo *primary = FindFirstAdvertisedInterface(m);
	AuthRecord *A = primary ? &primary->RR_A : mDNSNULL;
	for (intf = m->HostInterfaces; intf; intf = intf->next)
		if (intf->RR_A.RRSet == &set->RR_A)
			intf->RR_A.RRSet = A;

	// Unregister these records.
	// When doing the mDNS_Close processing, we first call mDNS_DeadvertiseInterface for each interface, so by the time the platform
	// support layer gets to call mDNS_DeregisterInterface, the address and PTR records have already been deregistered for it.
	// Also, in the event of a name conflict, one or more of our records will have been forcibly deregistered.
	// To avoid unnecessary and misleading warning messages, we check the RecordType before calling mDNS_Deregister_internal().
	if (set->RR_A.    resrec.RecordType) mDNS_Deregister_internal(m, &set->RR_A,     mDNS_Dereg_normal);
	if (set->RR_PTR.  resrec.RecordType) mDNS_Deregister_internal(m, &set->RR_PTR,   mDNS_Dereg_normal);
	if (set->RR_HINFO.resrec.RecordType) mDNS_Deregister_internal(m, &set->RR_HINFO, mDNS_Dereg_normal);
}

mDNSexport void mDNS_GenerateFQDN(mDNS *const m)
{
	domainname newname;
	mDNS_Lock(m);

	newname.c[0] = 0;
	if (!AppendDomainLabel(&newname, &m->hostlabel))  LogMsg("ERROR! Cannot create dot-local hostname");
	if (!AppendLiteralLabelString(&newname, "local")) LogMsg("ERROR! Cannot create dot-local hostname");
	if (!SameDomainName(&m->hostname, &newname))
	{
		NetworkInterfaceInfo *intf;
		AuthRecord *rr;

		m->hostname = newname;

		// 1. Stop advertising our address records on all interfaces
		for (intf = m->HostInterfaces; intf; intf = intf->next)
			if (intf->Advertise) mDNS_DeadvertiseInterface(m, intf);

		// 2. Start advertising our address records using the new name
		for (intf = m->HostInterfaces; intf; intf = intf->next)
			if (intf->Advertise) mDNS_AdvertiseInterface(m, intf);

		// 3. Make sure that any SRV records (and the like) that reference our
		// host name in their rdata get updated to reference this new host name
		for (rr = m->ResourceRecords;  rr; rr=rr->next) if (rr->HostTarget) SetTargetToHostName(m, rr);
		for (rr = m->DuplicateRecords; rr; rr=rr->next) if (rr->HostTarget) SetTargetToHostName(m, rr);
	}

	mDNS_Unlock(m);
}

mDNSlocal void HostNameCallback(mDNS *const m, AuthRecord *const rr, mStatus result)
{
	(void)rr;	// Unused parameter

#if MDNS_DEBUGMSGS
	{
		char *msg = "Unknown result";
		if      (result == mStatus_NoError)      msg = "Name registered";
		else if (result == mStatus_NameConflict) msg = "Name conflict";
		debugf("HostNameCallback: %##s (%s) %s (%ld)", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), msg, result);
	}
#endif

	if (result == mStatus_NoError)
	{
		// Notify the client that the host name is successfully registered
		if (m->MainCallback)
			m->MainCallback(m, result);
	}
	else if (result == mStatus_NameConflict)
	{
		domainlabel oldlabel = m->hostlabel;

		// 1. First give the client callback a chance to pick a new name
		if (m->MainCallback)
			m->MainCallback(m, mStatus_NameConflict);

		// 2. If the client callback didn't do it, add (or increment) an index ourselves
		if (SameDomainLabel(m->hostlabel.c, oldlabel.c))
			IncrementLabelSuffix(&m->hostlabel, mDNSfalse);

		// 3. Generate the FQDNs from the hostlabel,
		// and make sure all SRV records, etc., are updated to reference our new hostname
		mDNS_GenerateFQDN(m);
	}
}

mDNSlocal void UpdateInterfaceProtocols(mDNS *const m, NetworkInterfaceInfo *active)
{
	NetworkInterfaceInfo *intf;
	active->IPv4Available = mDNSfalse;
	active->IPv6Available = mDNSfalse;
	for (intf = m->HostInterfaces; intf; intf = intf->next)
		if (intf->InterfaceID == active->InterfaceID)
		{
			if (intf->ip.type == mDNSAddrType_IPv4 && intf->TxAndRx) active->IPv4Available = mDNStrue;
			if (intf->ip.type == mDNSAddrType_IPv6 && intf->TxAndRx) active->IPv6Available = mDNStrue;
		}
}

mDNSexport mStatus mDNS_RegisterInterface(mDNS *const m, NetworkInterfaceInfo *set)
{
	mDNSBool FirstOfType = mDNStrue;
	NetworkInterfaceInfo **p = &m->HostInterfaces;
	mDNS_Lock(m);
	
	// Assume this interface will be active
	set->InterfaceActive = mDNStrue;
	set->IPv4Available   = (set->ip.type == mDNSAddrType_IPv4 && set->TxAndRx);
	set->IPv6Available   = (set->ip.type == mDNSAddrType_IPv6 && set->TxAndRx);

	while (*p)
	{
		if (*p == set)
		{
			LogMsg("Error! Tried to register a NetworkInterfaceInfo that's already in the list");
			mDNS_Unlock(m);
			return(mStatus_AlreadyRegistered);
		}

		// This InterfaceID is already in the list, so mark this interface inactive for now
		if ((*p)->InterfaceID == set->InterfaceID)
		{
			set->InterfaceActive = mDNSfalse;
			if (set->ip.type == (*p)->ip.type) FirstOfType = mDNSfalse;
			if (set->ip.type == mDNSAddrType_IPv4 && set->TxAndRx) (*p)->IPv4Available = mDNStrue;
			if (set->ip.type == mDNSAddrType_IPv6 && set->TxAndRx) (*p)->IPv6Available = mDNStrue;
		}

		p=&(*p)->next;
	}

	set->next = mDNSNULL;
	*p = set;
	
	debugf("mDNS_RegisterInterface: InterfaceID %p %#a %s", set->InterfaceID, &set->ip,
	       set->InterfaceActive ?
	       "not represented in list; marking active and retriggering queries" :
	       "already represented in list; marking inactive for now");

	// In some versions of OS X the IPv6 address remains on an interface even when the interface is turned off,
	// giving the false impression that there's an active representative of this interface when there really isn't.
	// Therefore, when registering an interface, we want to re-trigger our questions and re-probe our Resource Records,
	// even if we believe that we previously had an active representative of this interface.
	if ((m->KnownBugs & mDNS_KnownBug_PhantomInterfaces) || FirstOfType || set->InterfaceActive)
	{
		DNSQuestion *q;
		AuthRecord *rr;
		// Use a small amount of randomness:
		// In the case of a network administrator turning on an Ethernet hub so that all the connected machines establish link at
		// exactly the same time, we don't want them to all go and hit the network with identical queries at exactly the same moment.
		if (!m->SuppressSending) m->SuppressSending = m->timenow + (mDNSs32)mDNSRandom((mDNSu32)InitialQuestionInterval);
		for (q = m->Questions; q; q=q->next)							// Scan our list of questions
			if (!q->InterfaceID || q->InterfaceID == set->InterfaceID)	// If non-specific Q, or Q on this specific interface,
			{														// then reactivate this question
				q->ThisQInterval = InitialQuestionInterval;				// MUST be > zero for an active question
				q->LastQTime     = m->timenow - q->ThisQInterval;
				q->RecentAnswers = 0;
				if (ActiveQuestion(q)) m->NextScheduledQuery = m->timenow;
			}
		
		// For all our non-specific authoritative resource records (and any dormant records specific to this interface)
		// we now need them to re-probe if necessary, and then re-announce.
		for (rr = m->ResourceRecords; rr; rr=rr->next)
			if (!rr->resrec.InterfaceID || rr->resrec.InterfaceID == set->InterfaceID)
			{
				if (rr->resrec.RecordType == kDNSRecordTypeVerified && !rr->DependentOn) rr->resrec.RecordType = kDNSRecordTypeUnique;
				rr->ProbeCount        = DefaultProbeCountForRecordType(rr->resrec.RecordType);
				if (rr->AnnounceCount < ReannounceCount)
					rr->AnnounceCount = ReannounceCount;
				rr->ThisAPInterval    = DefaultAPIntervalForRecordType(rr->resrec.RecordType);
				InitializeLastAPTime(m, rr);
			}
	}

	if (set->Advertise)
		mDNS_AdvertiseInterface(m, set);

	mDNS_Unlock(m);
	return(mStatus_NoError);
}

// NOTE: mDNS_DeregisterInterface calls mDNS_Deregister_internal which can call a user callback, which may change
// the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSexport void mDNS_DeregisterInterface(mDNS *const m, NetworkInterfaceInfo *set)
{
	NetworkInterfaceInfo **p = &m->HostInterfaces;
	
	mDNSBool revalidate = mDNSfalse;
	// If this platform has the "phantom interfaces" known bug (e.g. Jaguar), we have to revalidate records every
	// time an interface goes away. Otherwise, when you disconnect the Ethernet cable, the system reports that it
	// still has an IPv6 address, and if we don't revalidate those records don't get deleted in a timely fashion.
	if (m->KnownBugs & mDNS_KnownBug_PhantomInterfaces) revalidate = mDNStrue;

	mDNS_Lock(m);

	// Find this record in our list
	while (*p && *p != set) p=&(*p)->next;
	if (!*p) { debugf("mDNS_DeregisterInterface: NetworkInterfaceInfo not found in list"); mDNS_Unlock(m); return; }

	// Unlink this record from our list
	*p = (*p)->next;
	set->next = mDNSNULL;

	if (!set->InterfaceActive)
	{
		// If this interface not the active member of its set, update the v4/v6Available flags for the active member
		NetworkInterfaceInfo *intf;
		for (intf = m->HostInterfaces; intf; intf = intf->next)
			if (intf->InterfaceActive && intf->InterfaceID == set->InterfaceID)
				UpdateInterfaceProtocols(m, intf);
	}
	else
	{
		NetworkInterfaceInfo *intf;
		for (intf = m->HostInterfaces; intf; intf = intf->next)
			if (intf->InterfaceID == set->InterfaceID)
				break;
		if (intf)
		{
			debugf("mDNS_DeregisterInterface: Another representative of InterfaceID %p exists; making it active",
			       set->InterfaceID);
			intf->InterfaceActive = mDNStrue;
			UpdateInterfaceProtocols(m, intf);
			
			// See if another representative *of the same type* exists. If not, we mave have gone from
			// dual-stack to v6-only (or v4-only) so we need to reconfirm which records are still valid.
			for (intf = m->HostInterfaces; intf; intf = intf->next)
				if (intf->InterfaceID == set->InterfaceID && intf->ip.type == set->ip.type)
					break;
			if (!intf) revalidate = mDNStrue;
		}
		else
		{
			CacheRecord *rr;
			DNSQuestion *q;
			mDNSu32 slot;
			debugf("mDNS_DeregisterInterface: Last representative of InterfaceID %p deregistered; marking questions etc. dormant",
			       set->InterfaceID);

			// 1. Deactivate any questions specific to this interface
			for (q = m->Questions; q; q=q->next)
				if (q->InterfaceID == set->InterfaceID)
					q->ThisQInterval = 0;

			// 2. Flush any cache records received on this interface
			revalidate = mDNSfalse;		// Don't revalidate if we're flushing the records
			for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
				for (rr = m->rrcache_hash[slot]; rr; rr=rr->next)
					if (rr->resrec.InterfaceID == set->InterfaceID)
						PurgeCacheResourceRecord(m, rr);
		}
	}

	// If we were advertising on this interface, deregister those address and reverse-lookup records now
	if (set->Advertise)
		mDNS_DeadvertiseInterface(m, set);

	// If we have any cache records received on this interface that went away, then re-verify them.
	// In some versions of OS X the IPv6 address remains on an interface even when the interface is turned off,
	// giving the false impression that there's an active representative of this interface when there really isn't.
	// Don't need to do this when shutting down, because *all* interfaces are about to go away
	if (revalidate && !m->mDNS_shutdown)
	{
		mDNSu32 slot;
		CacheRecord *rr;
		m->NextCacheCheck = m->timenow;
		for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
			for (rr = m->rrcache_hash[slot]; rr; rr=rr->next)
				if (rr->resrec.InterfaceID == set->InterfaceID)
					mDNS_Reconfirm_internal(m, rr, kDefaultReconfirmTimeForCableDisconnect);
	}

	mDNS_Unlock(m);
}

mDNSlocal void ServiceCallback(mDNS *const m, AuthRecord *const rr, mStatus result)
{
	ServiceRecordSet *sr = (ServiceRecordSet *)rr->RecordContext;
	(void)m;	// Unused parameter

#if MDNS_DEBUGMSGS
	{
		char *msg = "Unknown result";
		if      (result == mStatus_NoError)      msg = "Name Registered";
		else if (result == mStatus_NameConflict) msg = "Name Conflict";
		else if (result == mStatus_MemFree)      msg = "Memory Free";
		debugf("ServiceCallback: %##s (%s) %s (%ld)", rr->resrec.name.c, DNSTypeName(rr->resrec.rrtype), msg, result);
	}
#endif

	// If we got a name conflict on either SRV or TXT, forcibly deregister this service, and record that we did that
	if (result == mStatus_NameConflict)
	{
		sr->Conflict = mDNStrue;							// Record that this service set had a conflict
		sr->RR_PTR.AnnounceCount = InitialAnnounceCount;	// Make sure we don't send a goodbye for the PTR record
		mDNS_DeregisterService(m, sr);						// Unlink the records from our list
		return;
	}
	
	if (result == mStatus_MemFree)
	{
		// If the PTR record or any of the subtype PTR record are still in the process of deregistering,
		// don't pass on the NameConflict/MemFree message until every record is finished cleaning up.
		mDNSu32 i;
		if (sr->RR_PTR.resrec.RecordType != kDNSRecordTypeUnregistered) return;
		for (i=0; i<sr->NumSubTypes; i++) if (sr->SubTypes[i].resrec.RecordType != kDNSRecordTypeUnregistered) return;

		// If this ServiceRecordSet was forcibly deregistered, and now its memory is ready for reuse,
		// then we can now report the NameConflict to the client
		if (sr->Conflict) result = mStatus_NameConflict;
	}

	// CAUTION: MUST NOT do anything more with sr after calling sr->Callback(), because the client's callback
	// function is allowed to do anything, including deregistering this service and freeing its memory.
	if (sr->ServiceCallback)
		sr->ServiceCallback(m, sr, result);
}

// Note:
// Name is first label of domain name (any dots in the name are actual dots, not label separators)
// Type is service type (e.g. "_printer._tcp.")
// Domain is fully qualified domain name (i.e. ending with a null label)
// We always register a TXT, even if it is empty (so that clients are not
// left waiting forever looking for a nonexistent record.)
// If the host parameter is mDNSNULL or the root domain (ASCII NUL),
// then the default host name (m->hostname1) is automatically used
mDNSexport mStatus mDNS_RegisterService(mDNS *const m, ServiceRecordSet *sr,
					const domainlabel *const name, const domainname *const type, const domainname *const domain,
					const domainname *const host, mDNSIPPort port, const mDNSu8 txtinfo[], mDNSu16 txtlen,
					AuthRecord *SubTypes, mDNSu32 NumSubTypes,
					const mDNSInterfaceID InterfaceID, mDNSServiceCallback Callback, void *Context)
{
	mStatus err;
	mDNSu32 i;

	sr->ServiceCallback = Callback;
	sr->ServiceContext  = Context;
	sr->Extras          = mDNSNULL;
	sr->NumSubTypes     = NumSubTypes;
	sr->SubTypes        = SubTypes;
	sr->Conflict        = mDNSfalse;
	if (host && host->c[0]) sr->Host = *host;
	else sr->Host.c[0] = 0;
	
	// Initialize the AuthRecord objects to sane values
	mDNS_SetupResourceRecord(&sr->RR_ADV, mDNSNULL, InterfaceID, kDNSType_PTR, kDefaultTTLforShared, kDNSRecordTypeAdvisory, ServiceCallback, sr);
	mDNS_SetupResourceRecord(&sr->RR_PTR, mDNSNULL, InterfaceID, kDNSType_PTR, kDefaultTTLforShared, kDNSRecordTypeShared,   ServiceCallback, sr);
	mDNS_SetupResourceRecord(&sr->RR_SRV, mDNSNULL, InterfaceID, kDNSType_SRV, kDefaultTTLforUnique, kDNSRecordTypeUnique,   ServiceCallback, sr);
	mDNS_SetupResourceRecord(&sr->RR_TXT, mDNSNULL, InterfaceID, kDNSType_TXT, kDefaultTTLforUnique, kDNSRecordTypeUnique,   ServiceCallback, sr);
	
	// If the client is registering an oversized TXT record,
	// it is the client's responsibility to alloate a ServiceRecordSet structure that is large enough for it
	if (sr->RR_TXT.resrec.rdata->MaxRDLength < txtlen)
		sr->RR_TXT.resrec.rdata->MaxRDLength = txtlen;

	// Set up the record names
	// For now we only create an advisory record for the main type, not for subtypes
	// We need to gain some operational experience before we decide if there's a need to create them for subtypes too
	if (ConstructServiceName(&sr->RR_ADV.resrec.name, (domainlabel*)"\x09_services", (domainname*)"\x07_dns-sd\x04_udp", domain) == mDNSNULL)
		return(mStatus_BadParamErr);
	if (ConstructServiceName(&sr->RR_PTR.resrec.name, mDNSNULL, type, domain) == mDNSNULL) return(mStatus_BadParamErr);
	if (ConstructServiceName(&sr->RR_SRV.resrec.name, name,     type, domain) == mDNSNULL) return(mStatus_BadParamErr);
	AssignDomainName(sr->RR_TXT.resrec.name, sr->RR_SRV.resrec.name);
	
	// 1. Set up the ADV record rdata to advertise our service type
	AssignDomainName(sr->RR_ADV.resrec.rdata->u.name, sr->RR_PTR.resrec.name);

	// 2. Set up the PTR record rdata to point to our service name
	// We set up two additionals, so when a client asks for this PTR we automatically send the SRV and the TXT too
	AssignDomainName(sr->RR_PTR.resrec.rdata->u.name, sr->RR_SRV.resrec.name);
	sr->RR_PTR.Additional1 = &sr->RR_SRV;
	sr->RR_PTR.Additional2 = &sr->RR_TXT;

	// 2a. Set up any subtype PTRs to point to our service name
	// If the client is using subtypes, it is the client's responsibility to have
	// already set the first label of the record name to the subtype being registered
	for (i=0; i<NumSubTypes; i++)
	{
		domainlabel s = *(domainlabel*)&sr->SubTypes[i].resrec.name;
		mDNS_SetupResourceRecord(&sr->SubTypes[i], mDNSNULL, InterfaceID, kDNSType_PTR, kDefaultTTLforShared, kDNSRecordTypeShared, ServiceCallback, sr);
		if (ConstructServiceName(&sr->SubTypes[i].resrec.name, &s, type, domain) == mDNSNULL) return(mStatus_BadParamErr);
		AssignDomainName(sr->SubTypes[i].resrec.rdata->u.name, sr->RR_SRV.resrec.name);
		sr->SubTypes[i].Additional1 = &sr->RR_SRV;
		sr->SubTypes[i].Additional2 = &sr->RR_TXT;
	}

	// 3. Set up the SRV record rdata.
	sr->RR_SRV.resrec.rdata->u.srv.priority = 0;
	sr->RR_SRV.resrec.rdata->u.srv.weight   = 0;
	sr->RR_SRV.resrec.rdata->u.srv.port     = port;

	// Setting HostTarget tells DNS that the target of this SRV is to be automatically kept in sync with our host name
	if (sr->Host.c[0]) AssignDomainName(sr->RR_SRV.resrec.rdata->u.srv.target, sr->Host);
	else sr->RR_SRV.HostTarget = mDNStrue;

	// 4. Set up the TXT record rdata,
	// and set DependentOn because we're depending on the SRV record to find and resolve conflicts for us
	if (txtinfo == mDNSNULL) sr->RR_TXT.resrec.rdlength = 0;
	else if (txtinfo != sr->RR_TXT.resrec.rdata->u.txt.c)
	{
		sr->RR_TXT.resrec.rdlength = txtlen;
		if (sr->RR_TXT.resrec.rdlength > sr->RR_TXT.resrec.rdata->MaxRDLength) return(mStatus_BadParamErr);
		mDNSPlatformMemCopy(txtinfo, sr->RR_TXT.resrec.rdata->u.txt.c, txtlen);
	}
	sr->RR_TXT.DependentOn = &sr->RR_SRV;

	mDNS_Lock(m);
	err = mDNS_Register_internal(m, &sr->RR_SRV);
	if (!err) err = mDNS_Register_internal(m, &sr->RR_TXT);
	// We register the RR_PTR last, because we want to be sure that in the event of a forced call to
	// mDNS_Close, the RR_PTR will be the last one to be forcibly deregistered, since that is what triggers
	// the mStatus_MemFree callback to ServiceCallback, which in turn passes on the mStatus_MemFree back to
	// the client callback, which is then at liberty to free the ServiceRecordSet memory at will. We need to
	// make sure we've deregistered all our records and done any other necessary cleanup before that happens.
	if (!err) err = mDNS_Register_internal(m, &sr->RR_ADV);
	for (i=0; i<NumSubTypes; i++) if (!err) err = mDNS_Register_internal(m, &sr->SubTypes[i]);
	if (!err) err = mDNS_Register_internal(m, &sr->RR_PTR);
	mDNS_Unlock(m);

	if (err) mDNS_DeregisterService(m, sr);
	return(err);
}

mDNSexport mStatus mDNS_AddRecordToService(mDNS *const m, ServiceRecordSet *sr,
					   ExtraResourceRecord *extra, RData *rdata, mDNSu32 ttl)
{
	mStatus result = mStatus_UnknownErr;
	ExtraResourceRecord **e = &sr->Extras;
	while (*e) e = &(*e)->next;

	// If TTL is unspecified, make it the same as the service's TXT and SRV default
	if (ttl == 0) ttl = kDefaultTTLforUnique;

	extra->next          = mDNSNULL;
	mDNS_SetupResourceRecord(&extra->r, rdata, sr->RR_PTR.resrec.InterfaceID, extra->r.resrec.rrtype, ttl, kDNSRecordTypeUnique, ServiceCallback, sr);
	AssignDomainName(extra->r.resrec.name, sr->RR_SRV.resrec.name);
	extra->r.DependentOn = &sr->RR_SRV;
	
	debugf("mDNS_AddRecordToService adding record to %##s", extra->r.resrec.name.c);
	
	result = mDNS_Register(m, &extra->r);
	if (!result) *e = extra;
	return result;
}

mDNSexport mStatus mDNS_RemoveRecordFromService(mDNS *const m, ServiceRecordSet *sr, ExtraResourceRecord *extra)
{
	ExtraResourceRecord **e = &sr->Extras;
	while (*e && *e != extra) e = &(*e)->next;
	if (!*e)
	{
		debugf("mDNS_RemoveRecordFromService failed to remove record from %##s", extra->r.resrec.name.c);
		return(mStatus_BadReferenceErr);
	}

	debugf("mDNS_RemoveRecordFromService removing record from %##s", extra->r.resrec.name.c);
	
	*e = (*e)->next;
	return(mDNS_Deregister(m, &extra->r));
}

mDNSexport mStatus mDNS_RenameAndReregisterService(mDNS *const m, ServiceRecordSet *const sr, const domainlabel *newname)
{
	domainlabel name;
	domainname type, domain;
	domainname *host = mDNSNULL;
	ExtraResourceRecord *extras = sr->Extras;
	mStatus err;

	DeconstructServiceName(&sr->RR_SRV.resrec.name, &name, &type, &domain);
	if (!newname)
	{
		IncrementLabelSuffix(&name, mDNStrue);
		newname = &name;
	}
	debugf("Reregistering as %#s", newname->c);
	if (sr->RR_SRV.HostTarget == mDNSfalse && sr->Host.c[0]) host = &sr->Host;
	
	err = mDNS_RegisterService(m, sr, newname, &type, &domain,
				   host, sr->RR_SRV.resrec.rdata->u.srv.port, sr->RR_TXT.resrec.rdata->u.txt.c, sr->RR_TXT.resrec.rdlength,
				   sr->SubTypes, sr->NumSubTypes,
				   sr->RR_PTR.resrec.InterfaceID, sr->ServiceCallback, sr->ServiceContext);

	// mDNS_RegisterService() just reset sr->Extras to NULL.
	// Fortunately we already grabbed ourselves a copy of this pointer (above), so we can now run
	// through the old list of extra records, and re-add them to our freshly created service registration
	while (!err && extras)
	{
		ExtraResourceRecord *e = extras;
		extras = extras->next;
		err = mDNS_AddRecordToService(m, sr, e, e->r.resrec.rdata, e->r.resrec.rroriginalttl);
	}
	
	return(err);
}

// NOTE: mDNS_DeregisterService calls mDNS_Deregister_internal which can call a user callback,
// which may change the record list and/or question list.
// Any code walking either list must use the CurrentQuestion and/or CurrentRecord mechanism to protect against this.
mDNSexport mStatus mDNS_DeregisterService(mDNS *const m, ServiceRecordSet *sr)
{
	if (sr->RR_PTR.resrec.RecordType == kDNSRecordTypeUnregistered)
	{
		debugf("Service set for %##s already deregistered", sr->RR_PTR.resrec.name.c);
		return(mStatus_BadReferenceErr);
	}
	else if (sr->RR_PTR.resrec.RecordType == kDNSRecordTypeDeregistering)
	{
		debugf("Service set for %##s already in the process of deregistering", sr->RR_PTR.resrec.name.c);
		return(mStatus_NoError);
	}
	else
	{
		mDNSu32 i;
		mStatus status;
		ExtraResourceRecord *e;
		mDNS_Lock(m);
		e = sr->Extras;
	
		// We use mDNS_Dereg_repeat because, in the event of a collision, some or all of the
		// SRV, TXT, or Extra records could have already been automatically deregistered, and that's okay
		mDNS_Deregister_internal(m, &sr->RR_SRV, mDNS_Dereg_repeat);
		mDNS_Deregister_internal(m, &sr->RR_TXT, mDNS_Dereg_repeat);
		
		mDNS_Deregister_internal(m, &sr->RR_ADV, mDNS_Dereg_normal);
	
		// We deregister all of the extra records, but we leave the sr->Extras list intact
		// in case the client wants to do a RenameAndReregister and reinstate the registration
		while (e)
		{
			mDNS_Deregister_internal(m, &e->r, mDNS_Dereg_repeat);
			e = e->next;
		}

		for (i=0; i<sr->NumSubTypes; i++)
			mDNS_Deregister_internal(m, &sr->SubTypes[i], mDNS_Dereg_normal);

		// Be sure to deregister the PTR last!
		// Deregistering this record is what triggers the mStatus_MemFree callback to ServiceCallback,
		// which in turn passes on the mStatus_MemFree (or mStatus_NameConflict) back to the client callback,
		// which is then at liberty to free the ServiceRecordSet memory at will. We need to make sure
		// we've deregistered all our records and done any other necessary cleanup before that happens.
		status = mDNS_Deregister_internal(m, &sr->RR_PTR, mDNS_Dereg_normal);
		mDNS_Unlock(m);
		return(status);
	}
}
#if 0
// Create a registration that asserts that no such service exists with this name.
// This can be useful where there is a given function is available through several protocols.
// For example, a printer called "Stuart's Printer" may implement printing via the "pdl-datastream" and "IPP"
// protocols, but not via "LPR". In this case it would be prudent for the printer to assert the non-existence of an
// "LPR" service called "Stuart's Printer". Without this precaution, another printer than offers only "LPR" printing
// could inadvertently advertise its service under the same name "Stuart's Printer", which might be confusing for users.
mDNSexport mStatus mDNS_RegisterNoSuchService(mDNS *const m, AuthRecord *const rr,
					      const domainlabel *const name, const domainname *const type, const domainname *const domain,
					      const domainname *const host,
					      const mDNSInterfaceID InterfaceID, mDNSRecordCallback Callback, void *Context)
{
	mDNS_SetupResourceRecord(rr, mDNSNULL, InterfaceID, kDNSType_SRV, kDefaultTTLforUnique, kDNSRecordTypeUnique, Callback, Context);
	if (ConstructServiceName(&rr->resrec.name, name, type, domain) == mDNSNULL) return(mStatus_BadParamErr);
	rr->resrec.rdata->u.srv.priority    = 0;
	rr->resrec.rdata->u.srv.weight      = 0;
	rr->resrec.rdata->u.srv.port        = zeroIPPort;
	if (host && host->c[0]) AssignDomainName(rr->resrec.rdata->u.srv.target, *host);
	else rr->HostTarget = mDNStrue;
	return(mDNS_Register(m, rr));
}

mDNSexport mStatus mDNS_AdvertiseDomains(mDNS *const m, AuthRecord *rr,
					 mDNS_DomainType DomainType, const mDNSInterfaceID InterfaceID, char *domname)
{
	mDNS_SetupResourceRecord(rr, mDNSNULL, InterfaceID, kDNSType_PTR, kDefaultTTLforShared, kDNSRecordTypeShared, mDNSNULL, mDNSNULL);
	if (!MakeDomainNameFromDNSNameString(&rr->resrec.name, mDNS_DomainTypeNames[DomainType])) return(mStatus_BadParamErr);
	if (!MakeDomainNameFromDNSNameString(&rr->resrec.rdata->u.name, domname))                 return(mStatus_BadParamErr);
	return(mDNS_Register(m, rr));
}
#endif
// ***************************************************************************
#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark -
#pragma mark -
#pragma mark - Startup and Shutdown
#endif

mDNSexport void mDNS_GrowCache(mDNS *const m, CacheRecord *storage, mDNSu32 numrecords)
{
	if (storage && numrecords)
	{
		mDNSu32 i;
		for (i=0; i<numrecords; i++) storage[i].next = &storage[i+1];
		storage[numrecords-1].next = m->rrcache_free;
		m->rrcache_free = storage;
		m->rrcache_size += numrecords;
	}
}

mDNSexport mStatus mDNS_Init(mDNS *const m, mDNS_PlatformSupport *const p,
			     CacheRecord *rrcachestorage, mDNSu32 rrcachesize,
			     mDNSBool AdvertiseLocalAddresses, mDNSCallback *Callback, void *Context)
{
	mDNSu32 slot;
	mDNSs32 timenow;
	mStatus result = mDNSPlatformTimeInit(&timenow);
	if (result != mStatus_NoError) return(result);
	
	if (!rrcachestorage) rrcachesize = 0;
	
	m->p                       = p;
	m->KnownBugs               = 0;
	m->AdvertiseLocalAddresses = AdvertiseLocalAddresses;
	m->mDNSPlatformStatus      = mStatus_Waiting;
	m->MainCallback            = Callback;
	m->MainContext             = Context;

	// For debugging: To catch and report locking failures
	m->mDNS_busy               = 0;
	m->mDNS_reentrancy         = 0;
	m->mDNS_shutdown           = mDNSfalse;
	m->lock_rrcache            = 0;
	m->lock_Questions          = 0;
	m->lock_Records            = 0;

	// Task Scheduling variables
	m->timenow                 = 0;		// MUST only be set within mDNS_Lock/mDNS_Unlock section
	m->timenow_last            = timenow;
	m->timenow_adjust          = 0;
	m->NextScheduledEvent      = timenow;
	m->SuppressSending         = timenow;
	m->NextCacheCheck          = timenow + kNextScheduledTime;
	m->NextScheduledQuery      = timenow + kNextScheduledTime;
	m->NextScheduledProbe      = timenow + kNextScheduledTime;
	m->NextScheduledResponse   = timenow + kNextScheduledTime;
	m->ExpectUnicastResponse   = timenow + kNextScheduledTime;
	m->RandomQueryDelay        = 0;
	m->SendDeregistrations     = mDNSfalse;
	m->SendImmediateAnswers    = mDNSfalse;
	m->SleepState              = mDNSfalse;

	// These fields only required for mDNS Searcher...
	m->Questions               = mDNSNULL;
	m->NewQuestions            = mDNSNULL;
	m->CurrentQuestion         = mDNSNULL;
	m->LocalOnlyQuestions      = mDNSNULL;
	m->NewLocalOnlyQuestions   = mDNSNULL;
	m->rrcache_size            = 0;
	m->rrcache_totalused       = 0;
	m->rrcache_active          = 0;
	m->rrcache_report          = 10;
	m->rrcache_free            = mDNSNULL;

	for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
	{
		m->rrcache_hash[slot] = mDNSNULL;
		m->rrcache_tail[slot] = &m->rrcache_hash[slot];
		m->rrcache_used[slot] = 0;
	}

	mDNS_GrowCache(m, rrcachestorage, rrcachesize);

	// Fields below only required for mDNS Responder...
	m->hostlabel.c[0]          = 0;
	m->nicelabel.c[0]          = 0;
	m->hostname.c[0]           = 0;
	m->HIHardware.c[0]         = 0;
	m->HISoftware.c[0]         = 0;
	m->ResourceRecords         = mDNSNULL;
	m->DuplicateRecords        = mDNSNULL;
	m->LocalOnlyRecords        = mDNSNULL;
	m->NewLocalOnlyRecords     = mDNSNULL;
	m->DiscardLocalOnlyRecords = mDNSfalse;
	m->CurrentRecord           = mDNSNULL;
	m->HostInterfaces          = mDNSNULL;
	m->ProbeFailTime           = 0;
	m->NumFailedProbes         = 0;
	m->SuppressProbes          = 0;

	result = mDNSPlatformInit(m);

	return(result);
}

mDNSexport void mDNSCoreInitComplete(mDNS *const m, mStatus result)
{
	m->mDNSPlatformStatus = result;
	if (m->MainCallback)
		m->MainCallback(m, mStatus_NoError);
}

mDNSexport void mDNS_Close(mDNS *const m)
{
	mDNSu32 rrcache_active = 0;
#if MDNS_DEBUGMSGS
	mDNSu32 rrcache_totalused = 0;
#endif
	mDNSu32 slot;
	NetworkInterfaceInfo *intf;
	mDNS_Lock(m);

	m->mDNS_shutdown = mDNStrue;
#if MDNS_DEBUGMSGS
	rrcache_totalused = m->rrcache_totalused;
#endif
	for (slot = 0; slot < CACHE_HASH_SLOTS; slot++)
	{
		while (m->rrcache_hash[slot])
		{
			CacheRecord *rr = m->rrcache_hash[slot];
			m->rrcache_hash[slot] = rr->next;
			if (rr->CRActiveQuestion) rrcache_active++;
			m->rrcache_used[slot]--;
			ReleaseCacheRR(m, rr);
		}
		// Reset tail pointer back to empty state (not that it really matters on exit, but we'll do it anyway, for the sake of completeness)
		m->rrcache_tail[slot] = &m->rrcache_hash[slot];
	}
	debugf("mDNS_Close: RR Cache was using %ld records, %d active", rrcache_totalused, rrcache_active);
	if (rrcache_active != m->rrcache_active)
		LogMsg("*** ERROR *** rrcache_active %lu != m->rrcache_active %lu", rrcache_active, m->rrcache_active);

	m->Questions = mDNSNULL;		// We won't be answering any more questions!
	
	for (intf = m->HostInterfaces; intf; intf = intf->next)
		if (intf->Advertise)
			mDNS_DeadvertiseInterface(m, intf);

	// Make sure there are nothing but deregistering records remaining in the list
	if (m->CurrentRecord) LogMsg("mDNS_Close ERROR m->CurrentRecord already set");
	m->CurrentRecord = m->ResourceRecords;
	while (m->CurrentRecord)
	{
		AuthRecord *rr = m->CurrentRecord;
		m->CurrentRecord = rr->next;
		if (rr->resrec.RecordType != kDNSRecordTypeDeregistering)
		{
			debugf("mDNS_Close: Record type %X still in ResourceRecords list %##s", rr->resrec.RecordType, rr->resrec.name.c);
			mDNS_Deregister_internal(m, rr, mDNS_Dereg_normal);
		}
	}

	if (m->ResourceRecords) debugf("mDNS_Close: Sending final packets for deregistering records");
	else debugf("mDNS_Close: No deregistering records remain");

	// If any deregistering records remain, send their deregistration announcements before we exit
	if (m->mDNSPlatformStatus != mStatus_NoError)
		DiscardDeregistrations(m);
	else
		while (m->ResourceRecords)
			SendResponses(m);
	
	mDNS_Unlock(m);
	debugf("mDNS_Close: mDNSPlatformClose");
	mDNSPlatformClose(m);
	debugf("mDNS_Close: done");
}
