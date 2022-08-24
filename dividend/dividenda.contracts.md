<h1 class="contract">
   clear   (TEST)
</h1>
**TEST**

**PARAMETERS:**

* __owner__ current NFT owner
* __new_owner__ new NFT owner
* __nftid__ NFT_id must be received from the front-end

**INTENT:** preformat and zeroes summary table - not erase, just make empty

**TERM:**  TESTING ONLY

<h1 class="contract">
   clear1  (TEST)
</h1>
**TEST**

**PARAMETERS:**

* __owner__ current NFT owner
* __new_owner__ new NFT owner
* __nftid__ NFT_id must be received from the front-end

**INTENT:** completely erases summary table

**TERM:**  TESTING ONLY

<h1 class="contract">
   dryrun
</h1>
<h2>simulate dividend run for one iteration using percentages only</h2>

**PARAMETERS:**
NONE
**PERMISSIONS**
Must be used proposer permission.
**INTENT:** The intent of [[dryrun]] is to run a whole process of dividend counting but using only percentage data taken from active NFTs.
Percentage of eligible payments (inactive (non-paid) NFTs are not counted) is summarized separately for each user. If user has more then one NFT the percentage of active NFTs will be sumarized. Locked users in cap 3 are counted as active as they will be active in the future.
Also percentage is summarized in a separate table for each category of payment e.g. Investors, Founders.
If total summary percentage of all payments will be above 90% the entering of a new proposal will be not allowed.
Also note that more information may be extracted using front end.  

**TERM:**

dividendhand( uint64_t periodpoint, asset profit )
<h1 class="contract">
   wraphand
</h1>
<h2>wrapper for dividendhand function</h2>

**PARAMETERS:**
* __periodpoint__ processed interaction number
* __profit__ amount of tokens to divide

**INTENT:** The intent of [[wraphand]] is to wrap the dividendhand function putting a security check before calling it.
The dividehand functions process one iteration. However parameter periodpoint is only used for memo. profit is the amount to divide as dividend.
The amount to divide is read from the deposit table, taken from freeosdivide account and distributed to eligible users without any further checking.   
dividehand function should be not possible to be called as an action.

**TERM:**

<h1 class="contract">
   replay
</h1>
<h2>process all iterations taken from deposit table except current one</h2>

**PARAMETERS:**
NONE.
**INTENT:** It will be called from front end as a dividend request without any checking. This action process divident only one per iteration except current independently on this how frequently was called.

**TERM:**

<h1 class="contract">
   proposalnew
</h1>
<h2>create new proposal</h2>

**PARAMETERS:**
* __proposer__ has right to initiate new proposal
* __eosaccount__ the dividend target account for founder or investor
* __roi_target_cap__ Value 1,2 or3. Itis 1-(I)teration, 2-(H)orizontal, or 3 for (V)ertical cap.
(* __expires_at__ proposal expiration time (default one hour). After that time proposal is cancelled and cannot be voted. Actually, used internally).
* __weekly_percentage__ of dividend transrerred into __eosaccount__ this week round
* __threshold__ different meaning depend on "roi_target_cap"=1 countdown counter of rounds to pay,  if 3 - the highest amount of dividend to pay this week
* __accrued__  total sum of dividends up-to-date - used only when roi_target_cap=2; if accrued>=threshold -> stop-pay
* __locked__ temporary lock for new founders (can be used only once)
Version 3 Feb
**INTENT:** The intent of [[proposalnew ]] is to create a new proposal introducing a new investor or founder to the register - When proposal is accepted the investor of founder receives NFT stored as register entry (NFT-Non Fungible Token) allowing to receive weekly pre-defined profit-based dividend.  The notification of a new proposal should be emailed to both voters by the frontend. The proposer account is priviledged (VIP) account.

**TERM:** The proposal must be accepted by pre-defined voters not later than one hour from its issue, otherwise will be destroyed.

<h1 class="contract">
   proposalvote
</h1>
<h2>vote on proposal</h2>

**PARAMETERS:**
* __voter__ is accountname (must be whitelisted) of a voter for identification
* __vote__ is a value of a vote ( 0-didn't voted yet or ignored, 1-no, 2-yes).

**INTENT:** The intent of proposalvote is to accept (or not) the issued proposal. If proposal will be accepted by both pre-defined priviledged voters, it will be copied to investor/founder register and NFT will be minted. From this moment investor/voter will be eligible to receive policy based share from dividends.
Otherwise, proposal not accepted by voters, will be destroyed.

**TERM:**  Voters have one hour (counted from proposal issue) to take the decision on proposal acceptance.

<h1 class="contract">
   propreset
</h1>
<h2>cancel proposal before expiration</h2>

**PARAMETERS:**
*NONE

**INTENT:** The intent of {{ proposalclr }} is to allow the proposer cancelling the pending proposal before its one hour normal expiration.

**TERM:** The voters will be notified on earlier proposal cancellation. Note: Accepted proposal becomes new NFT in the register and not exists anymore as a proposal.


<h1 class="contract">
   allowance
</h1>
<h2>Creates whitelist</h2>>

**PARAMETERS:**
* __key__ this is 1- for proposer, 2- for first voter, 3-for second voter.
* __vipname__ related proposer or voter account which will be used later for identification of proposer or voter.

**INTENT:** The intent of {{ allowance }} is to create a list of priviledged proposers and voters to create and accept the further proposals.  

**TERM:** The priviledged proposers and voters table allowed to deal with proposals is created only from command line and requires multisig.


<h1 class="contract">
   regchown
</h1>

**PARAMETERS:**

* __owner__ current NFT owner
* __new_owner__ new NFT owner
* __nftid__ NFT_id must be received from the front-end

**INTENT:** The intent of regchown is to allow changes in NFT ownership (e.g. only by inheritance or gift).

**TERM:**  TBA

<h1 class="contract">
   removeallow  (TEST)
</h1>
**TEST**

**PARAMETERS:**

NONE

**INTENT:** remove allowance table

**TERM:**  TESTING ONLY




<h1 class="contract">
   removedepos   (TEST)
</h1>
**TEST**

**PARAMETERS:**

* __iteration__ removes this iteration from local deposit table.

**INTENT:** Removes one row (pointed by iteration parameter) from local deposit table.

**TERM:**  TESTING ONLY


<h1 class="contract">
   removeonenft   (TEST)
</h1>
**TEST**

**PARAMETERS:**

* __nftid__ This NFT will be removed.

**INTENT:** Used to make surgical corrections to NFT register. It's removes NFT with given nft_id.

**TERM:**  TESTING ONLY




<h1 class="contract">
   setdeposit (TEST)
</h1>
**TEST**

**PARAMETERS:**

* __iter__ iteration
* __accr__ accrued


**INTENT:** Creates new entry of the local deposit table. Local deposit table is used to not use in test real external deposit table.
Also, internal table allows easier create of scenarios for testing.

**TERM:**  TESTING ONLY!


<h1 class="contract">
   zerofortest  (TEST)
</h1>
**TEST**

**PARAMETERS:**
NONE

**INTENT:** Removes all NFTs from the register. ONLY FOR TESTING.

**TERM:**  TESTING ONLY!!!


<h1 class="contract">
   version
</h1>

**PARAMETERS:**

NONE

**INTENT:** Display contract version.

**TERM:**  TBA


<h1 class="contract">
   votereset11   (TEST)
</h1>
**TEST**

**PARAMETERS:**
NONE

**INTENT:** Only removes voting results from the whitelist table.

**TERM:**  TESTING ONLY
