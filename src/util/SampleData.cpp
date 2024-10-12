#include "util/SampleData.hpp"

namespace chatterino {

/// Sample messages coming from IRC

const QStringList &getSampleCheerMessages()
{
    static QStringList list{
        R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=2;color=#FF0000;display-name=69_faith_420;emotes=;flags=;id=c5fd49c7-ecbc-46dd-a790-c9f10fdaaa67;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567282184553;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer2 Stop what? I'm not doing anything.)",
        R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=2;color=#FF0000;display-name=69_faith_420;emotes=;flags=;id=397f4d2e-cac8-4689-922a-32709b9e8b4f;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567282159076;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer2 Who keeps getting their bits out now?)",
        R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=2;color=#FF0000;display-name=FlameGodFlann;emotes=;flags=;id=664ddc92-649d-4889-9641-208a6e62ef1e;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567282066199;turbo=0;user-id=56442185;user-type= :flamegodflann!flamegodflann@flamegodflann.tmi.twitch.tv PRIVMSG #pajlada :Cheer2 I'm saving my only can of Stella for your upcoming win, lets go!)",
        R"(@badge-info=subscriber/3;badges=moderator/1,subscriber/3,bits/100;bits=10;color=#008000;display-name=k4izn;emotes=;flags=;id=3919af0b-93e0-412c-b238-d152f92ffea7;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567811485257;turbo=0;user-id=207114672;user-type=mod :k4izn!k4izn@k4izn.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Kleiner Cheer(s) !)",
        R"(@badge-info=subscriber/12;badges=subscriber/12,bits/1000;bits=20;color=#00CCFF;display-name=YaBoiBurnsy;emotes=;flags=;id=5b53975d-b339-484f-a2a0-3ffbedde0df2;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567529634584;turbo=0;user-id=45258137;user-type= :yaboiburnsy!yaboiburnsy@yaboiburnsy.tmi.twitch.tv PRIVMSG #pajlada :ShowLove20)",
        R"(@badge-info=subscriber/1;badges=moderator/1,subscriber/0,bits-leader/2;bits=1;color=;display-name=jdfellie;emotes=;flags=18-22:A.3/P.5;id=28c8f4b7-b1e3-4404-b0f8-5cfe46411ef9;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567668177856;turbo=0;user-id=137619637;user-type=mod :jdfellie!jdfellie@jdfellie.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 take a bit bitch)",
        R"(@badge-info=;badges=bits-leader/2;bits=30;color=#EC3B83;display-name=Sammay;emotes=;flags=;id=ccf058a6-c1f1-45de-a764-fc8f96f21449;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566719874294;turbo=0;user-id=58283830;user-type= :sammay!sammay@sammay.tmi.twitch.tv PRIVMSG #pajlada :ShowLove30 @Emperor_Zhang)",
        R"(@badge-info=;badges=bits-leader/2;bits=6;color=#97E7FF;display-name=Emperor_Zhang;emotes=;flags=;id=53bab01b-9f6c-4123-a852-9916ab371cf9;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566719803345;turbo=0;user-id=105292882;user-type= :emperor_zhang!emperor_zhang@emperor_zhang.tmi.twitch.tv PRIVMSG #pajlada :uni6)",
        R"(@badge-info=;badges=bits/1;bits=5;color=#97E7FF;display-name=Emperor_Zhang;emotes=;flags=;id=545caec6-8b5f-460a-8b4b-3e407e179689;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566704926380;turbo=0;user-id=105292882;user-type= :emperor_zhang!emperor_zhang@emperor_zhang.tmi.twitch.tv PRIVMSG #pajlada :VoHiYo5)",
        R"(@badge-info=;badges=bits/100;bits=50;color=;display-name=Schmiddi55;emotes=;flags=;id=777f1018-941d-48aa-bf4e-ed8053d556c8;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567708393343;turbo=0;user-id=101444120;user-type= :schmiddi55!schmiddi55@schmiddi55.tmi.twitch.tv PRIVMSG #pajlada :cheer50 sere ihr radlertrinker)",
        R"(@badge-info=subscriber/3;badges=subscriber/3,sub-gifter/10;bits=100;color=#0000FF;display-name=MLPTheChad;emotes=;flags=87-91:P.5;id=ed7db31e-884b-4761-9c88-b1676caa8814;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567681752733;turbo=0;user-id=63179867;user-type= :mlpthechad!mlpthechad@mlpthechad.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 Statistically speaking, 10 out of 10 constipated people don't give a shit.)",
        R"(@badge-info=subscriber/3;badges=subscriber/3,sub-gifter/10;bits=100;color=#0000FF;display-name=MLPTheChad;emotes=;flags=;id=506b482a-515a-4914-a694-2c69d2add23a;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567681618814;turbo=0;user-id=63179867;user-type= :mlpthechad!mlpthechad@mlpthechad.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 That's some SUB par gameplay, Dabier.)",
        R"(@badge-info=;badges=premium/1;bits=100;color=;display-name=AkiraKurusu__;emotes=;flags=;id=6e343f5d-0e0e-47f7-bf6d-d5d7bf18b95a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567765732657;turbo=0;user-id=151679027;user-type= :akirakurusu__!akirakurusu__@akirakurusu__.tmi.twitch.tv PRIVMSG #pajlada :TriHard100)",
        R"(@badge-info=;badges=premium/1;bits=1;color=;display-name=AkiraKurusu__;emotes=;flags=;id=dfdf6c2f-abee-4a4b-99fe-0d0b221f07de;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567765295301;turbo=0;user-id=151679027;user-type= :akirakurusu__!akirakurusu__@akirakurusu__.tmi.twitch.tv PRIVMSG #pajlada :TriHard1)",
        R"(@badge-info=;badges=bits/100;bits=500;color=#0000FF;display-name=Stabbr;emotes=;flags=;id=e28b384e-fb6a-4da5-9a36-1b6153c6089d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567648284623;turbo=0;user-id=183081176;user-type= :stabbr!stabbr@stabbr.tmi.twitch.tv PRIVMSG #pajlada :cheer500 Gotta be on top)",
        R"(@badge-info=subscriber/1;badges=subscriber/0,bits-leader/1;bits=100;color=;display-name=dbf_sub;emotes=;flags=;id=7cf317b8-6e28-4615-a0ba-e0bbaa0d4b29;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567646349560;turbo=0;user-id=450101746;user-type= :dbf_sub!dbf_sub@dbf_sub.tmi.twitch.tv PRIVMSG #pajlada :EleGiggle100)",
        R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=1;color=;display-name=dbf_sub;emotes=;flags=;id=43b5fc97-e7cc-4ac1-8d7e-7504c435c3f1;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567643510222;turbo=0;user-id=450101746;user-type= :dbf_sub!dbf_sub@dbf_sub.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood1)",
        R"(@badge-info=;badges=bits-leader/2;bits=100;color=;display-name=RobertsonRobotics;emotes=;flags=;id=598dfa14-23e9-4e45-a2fe-7a0263828817;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567873463820;turbo=0;user-id=117177721;user-type= :robertsonrobotics!robertsonrobotics@robertsonrobotics.tmi.twitch.tv PRIVMSG #pajlada :firstCheer100 This is so cool! Can’t wait for the competition!)",
        R"(@badge-info=;badges=bits/100;bits=18;color=#1E90FF;display-name=Vipacman11;emotes=;flags=;id=07f59664-0c75-459e-b137-26c8d03e44be;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567873210379;turbo=0;user-id=89634839;user-type= :vipacman11!vipacman11@vipacman11.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)",
        R"(@badge-info=;badges=sub-gifter/5;bits=100;color=#FF7F50;display-name=darkside_sinner;emotes=;flags=;id=090102b3-369d-4ce4-ad1f-283849b10de0;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567822075293;turbo=0;user-id=104942909;user-type= :darkside_sinner!darkside_sinner@darkside_sinner.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10)",
        R"(@badge-info=;badges=sub-gifter/5;bits=200;color=#FF7F50;display-name=darkside_sinner;emotes=;flags=;id=2bdf7846-5ffa-4798-a397-997e7209a6d0;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567821695287;turbo=0;user-id=104942909;user-type= :darkside_sinner!darkside_sinner@darkside_sinner.tmi.twitch.tv PRIVMSG #pajlada :Subway200 bonus20)",
        R"(@badge-info=;badges=bits/1;bits=50;color=#0000FF;display-name=SincereBC;emotes=;flags=;id=b8c9236b-aeb9-4c72-a191-593e33c6c3f1;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567818308913;turbo=0;user-id=146097597;user-type= :sincerebc!sincerebc@sincerebc.tmi.twitch.tv PRIVMSG #pajlada :cheer50)",
        R"(@badge-info=;badges=bits/1;bits=1;color=#FF0000;display-name=AngryCh33s3puff;emotes=;flags=;id=6ab62185-ac1b-4ee5-bd93-165009917078;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567474810480;turbo=0;user-id=55399500;user-type= :angrych33s3puff!angrych33s3puff@angrych33s3puff.tmi.twitch.tv PRIVMSG #pajlada :cheer1 for the chair!)",
        R"(@badge-info=subscriber/3;badges=moderator/1,subscriber/0,bits/1000;bits=1500;color=#5F9EA0;display-name=LaurenJW28;emotes=;flags=;id=2403678c-6109-43ac-b3b5-1f5230f91729;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567746107991;turbo=0;user-id=244354979;user-type=mod :laurenjw28!laurenjw28@laurenjw28.tmi.twitch.tv PRIVMSG #pajlada :Cheer1000 Cheer100 Cheer100 Cheer100 Cheer100 Cheer100)",
        R"(@badge-info=;badges=bits/1;bits=5;color=#5F9EA0;display-name=drkwings;emotes=;flags=;id=ad45dae5-b985-4526-9b9e-0bdba2d23289;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567742106689;turbo=0;user-id=440230526;user-type= :drkwings!drkwings@drkwings.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood1 SeemsGood1 SeemsGood1 SeemsGood1 SeemsGood1)",
        R"(@badge-info=subscriber/16;badges=subscriber/12,bits/1000;bits=1;color=;display-name=mustangbugatti;emotes=;flags=;id=ee987ee9-46a4-4c06-bf66-2cafff5d4cdd;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567883658780;turbo=0;user-id=115948494;user-type= :mustangbugatti!mustangbugatti@mustangbugatti.tmi.twitch.tv PRIVMSG #pajlada :(In clarkson accent) Some say...the only number in his contacts is himself..... And...that he is the international butt-dial champion... All we know is.... HES CALLED THE STIG Cheer1)",
        R"(@badge-info=subscriber/2;badges=subscriber/0,bits/1000;bits=1;color=;display-name=derpysaurus1;emotes=;flags=;id=c41c3d8b-c591-4db0-87e7-a78c5536de82;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567883655116;turbo=0;user-id=419221818;user-type= :derpysaurus1!derpysaurus1@derpysaurus1.tmi.twitch.tv PRIVMSG #pajlada :cheer1 OMG ur back yaaaaaaaaaaaaaaaaaaaaayyyyyyyyy)",
        R"(@badge-info=subscriber/5;badges=subscriber/0,premium/1;bits=1;color=#8A2BE2;display-name=sirlordstallion;emotes=;flags=;id=61a87aeb-88b1-42f9-90f5-74429d8bf387;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882978939;turbo=0;user-id=92145441;user-type= :sirlordstallion!sirlordstallion@sirlordstallion.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Alex is definetly not putting his eggs in Narreths basket)",
        R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=1;color=;display-name=xplosivegingerx;emotes=;flags=;id=f8aac1e0-050a-44bf-abcc-c0cf12cbedfc;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882249072;turbo=0;user-id=151265906;user-type= :xplosivegingerx!xplosivegingerx@xplosivegingerx.tmi.twitch.tv PRIVMSG #pajlada :Cheer1)",
        R"(@badge-info=;badges=bits/100;bits=500;color=;display-name=AlexJohanning;emotes=;flags=;id=4e4229a3-e7f2-4082-8c55-47d42db3b09c;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567881969862;turbo=0;user-id=190390930;user-type= :alexjohanning!alexjohanning@alexjohanning.tmi.twitch.tv PRIVMSG #pajlada :cheer500)",
        R"(@badge-info=;badges=bits-leader/1;bits=245;color=;display-name=undonebunion6;emotes=;flags=;id=331ec583-0a80-4299-9206-0efd9e33d934;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567881553759;turbo=0;user-id=452974274;user-type= :undonebunion6!undonebunion6@undonebunion6.tmi.twitch.tv PRIVMSG #pajlada :cheer245 can I join?)",
        R"(@badge-info=;badges=bits/100;bits=100;color=;display-name=therealruffnix;emotes=;flags=61-67:S.6;id=25f567ad-ac95-45ab-b12e-4d647f6a2345;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567524218162;turbo=0;user-id=55059620;user-type= :therealruffnix!therealruffnix@therealruffnix.tmi.twitch.tv PRIVMSG #pajlada :cheer100 This is the kind of ASMR I'm missing on YouTube and PornHub)",
        R"(@badge-info=;badges=bits/1;bits=1;color=;display-name=BeamMeUpSnotty;emotes=;flags=;id=8022f41f-dcb8-42f2-b46a-04d4a99180bd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567270037926;turbo=0;user-id=261679182;user-type= :beammeupsnotty!beammeupsnotty@beammeupsnotty.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood1)",
        R"(@badge-info=;badges=bits/1;bits=10;color=#00FF7F;display-name=EXDE_HUN;emotes=;flags=;id=60d8835b-23fa-418c-96ca-5874e5d5e8ba;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566654664248;turbo=0;user-id=129793695;user-type= :exde_hun!exde_hun@exde_hun.tmi.twitch.tv PRIVMSG #pajlada :PogChamp10)",
        R"(@badge-info=;badges=bits-leader/3;bits=5;color=;display-name=slyckity;emotes=;flags=;id=fd6c5507-3a4e-4d24-8f6e-fadf07f520d3;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824273752;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)",
        R"(@badge-info=;badges=bits-leader/3;bits=5;color=;display-name=slyckity;emotes=;flags=;id=7003f119-b9a6-4319-a1e8-8e99f96ab01a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824186437;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)",
        R"(@badge-info=;badges=bits-leader/3;bits=10;color=;display-name=slyckity;emotes=;flags=;id=3f7de686-77f6-46d2-919e-404312c6676f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824128736;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)",
        R"(@badge-info=;badges=bits-leader/3;bits=10;color=;display-name=slyckity;emotes=;flags=;id=9e830ed3-8735-4ccb-9a8b-80466598ca19;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824118921;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)",
        R"(@badge-info=;badges=bits-leader/1;bits=377;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=262f4d54-9b21-4f13-aac3-6d3b1051282f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440897074;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :NotLikeThis377)",
        R"(@badge-info=;badges=bits-leader/1;bits=144;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=3556e0ad-b5f8-4190-9c4c-e39c1940d191;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440861545;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :bday144)",
        R"(@badge-info=;badges=bits-leader/1;bits=89;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=96e380a5-786d-44b8-819a-529b6adb06ac;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440848361;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :SwiftRage89)",
        R"(@badge-info=;badges=bits-leader/1;bits=34;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=76239011-65fa-4f6a-a6d6-dc5d5dcbd674;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440816630;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :MrDestructoid34)",
        R"(@badge-info=;badges=bits-leader/1;bits=21;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=4c05c97c-7b6c-4ae9-bc91-04e98240c1d5;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440806389;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :TriHard21)",
        R"(@badge-info=;badges=bits-leader/1;bits=8;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=3b2ecce7-842e-429e-b6c8-9456c4646362;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440774009;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :EleGiggle8)",
        R"(@badge-info=;badges=bits-leader/1;bits=5;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=3b8736d1-832d-4152-832a-50c526714fd1;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440762580;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :uni5)",
        R"(@badge-info=;badges=bits-leader/1;bits=3;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=c13a1540-2a03-4c7d-af50-cb20ed88cefd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440750103;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :Party3)",
        R"(@badge-info=;badges=bits-leader/1;bits=2;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=5d889eeb-b6b9-4a4e-91ff-0aecdf297edd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440738337;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :ShowLove2)",
        R"(@badge-info=;badges=bits/1;bits=1;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=da47f91a-40d3-4209-ba1c-0219d8b8ecaf;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440720363;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :Scoops1)",
        R"(@badge-info=;badges=bits/1;bits=10;color=#8A2BE2;display-name=EkimSky;emotes=;flags=;id=8adea5b4-7430-44ea-a666-5ebaceb69441;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567833047623;turbo=0;user-id=42132818;user-type= :ekimsky!ekimsky@ekimsky.tmi.twitch.tv PRIVMSG #pajlada :Hi Cheer10)",
        R"(@badge-info=;badges=bits-leader/2;bits=500;color=;display-name=godkiller76;emotes=;flags=;id=80e86bcc-d048-44f3-8073-9a1014568e0c;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567753685704;turbo=0;user-id=258838478;user-type= :godkiller76!godkiller76@godkiller76.tmi.twitch.tv PRIVMSG #pajlada :Party100 Party100 Party100 Party100 Party100)",
        R"(@mod=0;flags=;badge-info=;source-badge-info=;color=#DAA520;user-id=612865661;subscriber=0;id=886028cc-9985-47b9-a273-8164c6d59a76;turbo=0;source-badges=staff/1,moderator/1,twitch-recap-2023/1;room-id=11148817;source-id=eefbae4a-d3a1-4307-8d15-fab0f03fd9b9;source-room-id=1025594235;emotes=;display-name=lahoooo;tmi-sent-ts=1727304317562;badges=staff/1,raging-wolf-helm/1;user-type=staff;bits=1 :lahoooo!lahoooo@lahoooo.tmi.twitch.tv PRIVMSG #pajlada Cheer1)",
        R"(@id=7bf90f3f-75de-4e89-ab3d-2fdfefd6bfb1;source-id=590821fd-4a5c-4dd8-b27e-9cea4ffb8d87;source-badges=staff/1,moderator/1,bits-leader/3;user-id=612865661;badges=staff/1,raging-wolf-helm/1;emotes=;source-badge-info=;badge-info=;mod=0;source-room-id=1025594235;user-type=staff;color=#DAA520;tmi-sent-ts=1727375798676;display-name=lahoooo;bits=1;flags=;turbo=0;room-id=11148817;subscriber=0 :lahoooo!lahoooo@lahoooo.tmi.twitch.tv PRIVMSG #pajlada shared9Cheer1)",
    };
    return list;
}

const QStringList &getSampleSubMessages()
{
    static QStringList list{
        R"(@badges=staff/1,broadcaster/1,turbo/1;color=#008000;display-name=ronni;emotes=;id=db25007f-7a18-43eb-9379-80131e44d633;login=ronni;mod=0;msg-id=resub;msg-param-months=6;msg-param-sub-plan=Prime;msg-param-sub-plan-name=Prime;room-id=11148817;subscriber=1;system-msg=ronni\shas\ssubscribed\sfor\s6\smonths!;tmi-sent-ts=1507246572675;turbo=1;user-id=1337;user-type=staff :tmi.twitch.tv USERNOTICE #pajlada :Great stream -- keep it up!)",
        R"(@badges=staff/1,premium/1;color=#0000FF;display-name=TWW2;emotes=;id=e9176cd8-5e22-4684-ad40-ce53c2561c5e;login=tww2;mod=0;msg-id=subgift;msg-param-months=1;msg-param-recipient-display-name=Mr_Woodchuck;msg-param-recipient-id=89614178;msg-param-recipient-name=mr_woodchuck;msg-param-sub-plan-name=House\sof\sNyoro~n;msg-param-sub-plan=1000;room-id=19571752;subscriber=0;system-msg=TWW2\sgifted\sa\sTier\s1\ssub\sto\sMr_Woodchuck!;tmi-sent-ts=1521159445153;turbo=0;user-id=13405587;user-type=staff :tmi.twitch.tv USERNOTICE #pajlada)",

        // hyperbolicxd gifted a sub to quote_if_nam
        R"(@badges=subscriber/0,premium/1;color=#00FF7F;display-name=hyperbolicxd;emotes=;id=b20ef4fe-cba8-41d0-a371-6327651dc9cc;login=hyperbolicxd;mod=0;msg-id=subgift;msg-param-months=1;msg-param-recipient-display-name=quote_if_nam;msg-param-recipient-id=217259245;msg-param-recipient-user-name=quote_if_nam;msg-param-sender-count=1;msg-param-sub-plan-name=Channel\sSubscription\s(nymn_hs);msg-param-sub-plan=1000;room-id=62300805;subscriber=1;system-msg=hyperbolicxd\sgifted\sa\sTier\s1\ssub\sto\squote_if_nam!\sThis\sis\stheir\sfirst\sGift\sSub\sin\sthe\schannel!;tmi-sent-ts=1528190938558;turbo=0;user-id=111534250;user-type= :tmi.twitch.tv USERNOTICE #pajlada)",

        // multi-month sub gift
        R"(@badge-info=subscriber/32;badges=subscriber/3030,sub-gift-leader/2;color=#FF8EA3;display-name=iNatsuFN;emotes=;flags=;id=0d0decbd-b8f4-4e83-9e18-eca9cab69153;login=inatsufn;mod=0;msg-id=subgift;msg-param-gift-months=6;msg-param-goal-contribution-type=SUBS;msg-param-goal-current-contributions=881;msg-param-goal-target-contributions=900;msg-param-goal-user-contributions=1;msg-param-months=16;msg-param-origin-id=2524053421157386961;msg-param-recipient-display-name=kimmi_tm;msg-param-recipient-id=225806893;msg-param-recipient-user-name=kimmi_tm;msg-param-sender-count=334;msg-param-sub-plan-name=Channel\sSubscription\s(mxddy);msg-param-sub-plan=1000;room-id=210915729;subscriber=1;system-msg=iNatsuFN\sgifted\s6\smonths\sof\sTier\s1\sto\skimmi_tm.\sThey've\sgifted\s334\smonths\sin\sthe\schannel!;tmi-sent-ts=1712034497332;user-id=218205938;user-type=;vip=0 :tmi.twitch.tv USERNOTICE #pajlada)",

        // multi-month anon sub gift
        R"(@msg-param-goal-user-contributions=1;system-msg=An\sanonymous\suser\sgifted\sa\sTier\s1\ssub\sto\sMohammadrezaDH!\s;msg-param-goal-current-contributions=2;vip=0;color=;user-id=274598607;mod=0;flags=;msg-param-months=2;historical=1;id=afa2155b-f563-4973-a5c2-e4075882bbfb;msg-param-gift-months=6;msg-id=subgift;badge-info=;msg-param-recipient-user-name=mohammadrezadh;login=ananonymousgifter;room-id=441388138;msg-param-goal-target-contributions=25;rm-received-ts=1712002037736;msg-param-recipient-id=204174899;emotes=;display-name=AnAnonymousGifter;badges=;msg-param-fun-string=FunStringFive;msg-param-goal-contribution-type=NEW_SUB_POINTS;msg-param-origin-id=8862142563198473546;msg-param-recipient-display-name=MohammadrezaDH;msg-param-sub-plan-name=jmarxists;user-type=;subscriber=0;tmi-sent-ts=1712002037615;msg-param-sub-plan=1000;msg-param-goal-description=day\slee\sgoal\s:-) :tmi.twitch.tv USERNOTICE #pajlada)",

        // multi-month sub gift by broadcaster
        R"(@user-id=35759863;msg-param-origin-id=2862055070165643340;display-name=Lucidfoxx;id=eeb3cdb8-337c-413a-9521-3a884ff78754;msg-param-gift-months=12;msg-param-sub-plan=1000;vip=0;emotes=;badges=broadcaster/1,subscriber/3042,partner/1;msg-param-recipient-user-name=ogprodigy;msg-param-recipient-id=53888434;badge-info=subscriber/71;room-id=35759863;msg-param-recipient-display-name=OGprodigy;msg-param-sub-plan-name=Silver\sPackage;subscriber=1;system-msg=Lucidfoxx\sgifted\sa\sTier\s1\ssub\sto\sOGprodigy!;login=lucidfoxx;msg-param-sender-count=0;user-type=;mod=0;flags=;rm-received-ts=1712803947891;color=#EB078D;msg-param-months=15;tmi-sent-ts=1712803947773;msg-id=subgift :tmi.twitch.tv USERNOTICE #pajlada)",

        // multi-month sub
        R"(@badge-info=subscriber/1;badges=subscriber/0;emotes=;msg-param-sub-plan=1000;msg-param-months=0;mod=0;login=foly__;room-id=11148817;flags=;user-id=441166175;msg-param-multimonth-tenure=0;msg-param-should-share-streak=0;system-msg=foly__\ssubscribed\sat\sTier\s1.;msg-id=sub;display-name=foly__;msg-param-sub-plan-name=Channel\sSubscription\s(k4sen);user-type=;id=327249bb-81bc-4f87-8b43-c05720a2dd64;msg-param-was-gifted=false;tmi-sent-ts=1728710962985;msg-param-cumulative-months=1;vip=0;color=;subscriber=1;msg-param-multimonth-duration=6 :tmi.twitch.tv USERNOTICE #pajlada)",

        // multi-month resub at tier 3
        R"(@system-msg=calm__like_a_tom\ssubscribed\sat\sTier\s3.\sThey've\ssubscribed\sfor\s9\smonths!;msg-param-cumulative-months=9;mod=0;msg-param-should-share-streak=0;msg-param-sub-plan-name=Executive\sProducer;color=#0000FF;msg-param-months=0;msg-param-multimonth-duration=6;user-type=;flags=;msg-id=resub;user-id=609242230;room-id=11148817;msg-param-multimonth-tenure=0;msg-param-sub-plan=3000;emotes=;badge-info=subscriber/9;msg-param-was-gifted=false;id=4a6e270c-8cdb-46e9-b602-f8177a79d472;badges=subscriber/3009;display-name=calm__like_a_tom;tmi-sent-ts=1725938011176;login=calm__like_a_tom;vip=0;subscriber=1 :tmi.twitch.tv USERNOTICE #pajlada)",

        // first time sub
        R"(@badges=subscriber/0,premium/1;color=#0000FF;display-name=byebyeheart;emotes=;id=fe390424-ab89-4c33-bb5a-53c6e5214b9f;login=byebyeheart;mod=0;msg-id=sub;msg-param-months=0;msg-param-sub-plan-name=Dakotaz;msg-param-sub-plan=Prime;room-id=39298218;subscriber=0;system-msg=byebyeheart\sjust\ssubscribed\swith\sTwitch\sPrime!;tmi-sent-ts=1528190963670;turbo=0;user-id=131956000;user-type= :tmi.twitch.tv USERNOTICE #pajlada)",

        // first time sub
        R"(@badges=subscriber/0,premium/1;color=;display-name=vJoeyzz;emotes=;id=b2476df5-fffe-4338-837b-380c5dd90051;login=vjoeyzz;mod=0;msg-id=sub;msg-param-months=0;msg-param-sub-plan-name=Dakotaz;msg-param-sub-plan=Prime;room-id=39298218;subscriber=0;system-msg=vJoeyzz\sjust\ssubscribed\swith\sTwitch\sPrime!;tmi-sent-ts=1528190995089;turbo=0;user-id=78945903;user-type= :tmi.twitch.tv USERNOTICE #pajlada)",

        // first time sub
        R"(@badges=subscriber/0,premium/1;color=;display-name=Lennydog3;emotes=;id=44feb1eb-df60-45f6-904b-7bf0d5375a41;login=lennydog3;mod=0;msg-id=sub;msg-param-months=0;msg-param-sub-plan-name=Dakotaz;msg-param-sub-plan=Prime;room-id=39298218;subscriber=0;system-msg=Lennydog3\sjust\ssubscribed\swith\sTwitch\sPrime!;tmi-sent-ts=1528191098733;turbo=0;user-id=175759335;user-type= :tmi.twitch.tv USERNOTICE #pajlada)",

        // resub with message
        R"(@badges=subscriber/0,premium/1;color=#1E90FF;display-name=OscarLord;emotes=;id=376529fd-31a8-4da9-9c0d-92a9470da2cd;login=oscarlord;mod=0;msg-id=resub;msg-param-months=2;msg-param-sub-plan-name=Dakotaz;msg-param-sub-plan=1000;room-id=39298218;subscriber=1;system-msg=OscarLord\sjust\ssubscribed\swith\sa\sTier\s1\ssub.\sOscarLord\ssubscribed\sfor\s2\smonths\sin\sa\srow!;tmi-sent-ts=1528191154801;turbo=0;user-id=162607810;user-type= :tmi.twitch.tv USERNOTICE #pajlada :Hey dk love to watch your streams keep up the good work)",

        // resub with message
        R"(@badges=subscriber/0,premium/1;color=;display-name=samewl;emotes=9:22-23;id=599fda87-ca1e-41f2-9af7-6a28208daf1c;login=samewl;mod=0;msg-id=resub;msg-param-months=5;msg-param-sub-plan-name=Channel\sSubscription\s(forsenlol);msg-param-sub-plan=Prime;room-id=22484632;subscriber=1;system-msg=samewl\sjust\ssubscribed\swith\sTwitch\sPrime.\ssamewl\ssubscribed\sfor\s5\smonths\sin\sa\srow!;tmi-sent-ts=1528191317948;turbo=0;user-id=70273207;user-type= :tmi.twitch.tv USERNOTICE #pajlada :lot of love sebastian <3)",

        // resub without message
        R"(@badges=subscriber/12;color=#CC00C2;display-name=cspice;emotes=;id=6fc4c3e0-ca61-454a-84b8-5669dee69fc9;login=cspice;mod=0;msg-id=resub;msg-param-months=12;msg-param-sub-plan-name=Channel\sSubscription\s(forsenlol):\s$9.99\sSub;msg-param-sub-plan=2000;room-id=22484632;subscriber=1;system-msg=cspice\sjust\ssubscribed\swith\sa\sTier\s2\ssub.\scspice\ssubscribed\sfor\s12\smonths\sin\sa\srow!;tmi-sent-ts=1528192510808;turbo=0;user-id=47894662;user-type= :tmi.twitch.tv USERNOTICE #pajlada)",
    };
    return list;
}

const QStringList &getSampleMiscMessages()
{
    static QStringList list{
        // display name renders strangely
        R"(@badges=;color=#00AD2B;display-name=Iamme420\s;emotes=;id=d47a1e4b-a3c6-4b9e-9bf1-51b8f3dbc76e;mod=0;room-id=11148817;subscriber=0;tmi-sent-ts=1529670347537;turbo=0;user-id=56422869;user-type= :iamme420!iamme420@iamme420.tmi.twitch.tv PRIVMSG #pajlada :offline chat gachiBASS)",
        R"(@badge-info=founder/47;badges=moderator/1,founder/0,premium/1;color=#00FF80;display-name=gempir;emotes=;flags=;id=d4514490-202e-43cb-b429-ef01a9d9c2fe;mod=1;room-id=11148817;subscriber=0;tmi-sent-ts=1575198233854;turbo=0;user-id=77829817;user-type=mod :gempir!gempir@gempir.tmi.twitch.tv PRIVMSG #pajlada :offline chat gachiBASS)",

        // "first time chat" message
        R"(@badge-info=;badges=glhf-pledge/1;client-nonce=5d2627b0cbe56fa05faf5420def4807d;color=#1E90FF;display-name=oldcoeur;emote-only=1;emotes=84608:0-7;first-msg=1;flags=;id=7412fea4-8683-4cc9-a506-4228127a5c2d;mod=0;room-id=11148817;subscriber=0;tmi-sent-ts=1623429859222;turbo=0;user-id=139147886;user-type= :oldcoeur!oldcoeur@oldcoeur.tmi.twitch.tv PRIVMSG #pajlada :cmonBruh)",

        // Message with founder badge
        R"(@badge-info=founder/72;badges=founder/0,bits/5000;color=#FF0000;display-name=TranRed;emotes=;first-msg=0;flags=;id=7482163f-493d-41d9-b36f-fba50e0701b7;mod=0;room-id=11148817;subscriber=0;tmi-sent-ts=1641123773885;turbo=0;user-id=57019243;user-type= :tranred!tranred@tranred.tmi.twitch.tv PRIVMSG #pajlada :GFMP pajaE)",

        // mod announcement
        R"(@badge-info=subscriber/47;badges=broadcaster/1,subscriber/3012,twitchconAmsterdam2020/1;color=#FF0000;display-name=Supinic;emotes=;flags=;id=8c26e1ab-b50c-4d9d-bc11-3fd57a941d90;login=supinic;mod=0;msg-id=announcement;msg-param-color=PRIMARY;room-id=31400525;subscriber=1;system-msg=;tmi-sent-ts=1648762219962;user-id=31400525;user-type= :tmi.twitch.tv USERNOTICE #supinic :mm test lol)",

        // mod announcement from another channel
        R"(@badge-info=;badges=staff/1,raging-wolf-helm/1;color=#DAA520;display-name=lahoooo;emotes=;flags=;id=01cd601f-bc3f-49d5-ab4b-136fa9d6ec22;login=lahoooo;mod=0;msg-id=sharedchatnotice;msg-param-color=PRIMARY;room-id=11148817;source-badge-info=;source-badges=staff/1,moderator/1,bits-leader/1;source-id=4083dadc-9f20-40f9-ba92-949ebf6bc294;source-msg-id=announcement;source-room-id=1025594235;subscriber=0;system-msg=;tmi-sent-ts=1726118378465;user-id=612865661;user-type=staff;vip=0 :tmi.twitch.tv USERNOTICE #pajlada :hi this is an announcement from 1)",

        // shared chat message
        R"(@badge-info=;flags=;room-id=11148817;color=;client-nonce=0d1632f37b6baee51576859d5dbaf325;emotes=;subscriber=0;tmi-sent-ts=1727395701680;id=19ee1663-c14d-41cd-a4a2-30a4bb609c5a;turbo=1;badges=staff/1,turbo/1;source-badges=staff/1,moderator/1,bits-leader/1;source-badge-info=;display-name=creativewind;source-room-id=1025594235;source-id=b97eea45-f9dc-4f0c-8744-f8256c3ed950;user-type=staff;user-id=106940612;mod=0 :creativewind!creativewind@creativewind.tmi.twitch.tv PRIVMSG #pajlada :Guys can you please not share the chat. My mom bought me this new laptop and it gets really hot when the chat is being shared. Now my leg is starting to hurt because it is getting so hot. Please, if you don't want me to get burned, then dont share the chat.)",

        // Hype Chat (Paid option for keeping a message in chat longer)
        // no level
        R"(@badge-info=subscriber/3;badges=subscriber/0,bits-charity/1;color=#0000FF;display-name=SnoopyTheBot;emotes=;first-msg=0;flags=;id=8779a9e5-cf1b-47b3-b9fe-67a5b1b605f6;mod=0;pinned-chat-paid-amount=500;pinned-chat-paid-canonical-amount=5;pinned-chat-paid-currency=USD;pinned-chat-paid-exponent=2;returning-chatter=0;room-id=36340781;subscriber=1;tmi-sent-ts=1664505974154;turbo=0;user-id=136881249;user-type= :snoopythebot!snoopythebot@snoopythebot.tmi.twitch.tv PRIVMSG #pajlada :-$5)",
        // level 1
        R"(@pinned-chat-paid-level=ONE;mod=0;flags=;pinned-chat-paid-amount=1400;pinned-chat-paid-exponent=2;tmi-sent-ts=1687970631828;subscriber=1;user-type=;color=#9DA364;emotes=;badges=predictions/blue-1,subscriber/60,twitchconAmsterdam2020/1;pinned-chat-paid-canonical-amount=1400;turbo=0;user-id=26753388;id=e6681ba0-cdc6-4482-93a3-515b74361e8b;room-id=36340781;first-msg=0;returning-chatter=0;pinned-chat-paid-currency=NOK;pinned-chat-paid-is-system-message=0;badge-info=predictions/Day\s53/53\sforsenSmug,subscriber/67;display-name=matrHS :matrhs!matrhs@matrhs.tmi.twitch.tv PRIVMSG #pajlada :Title: Beating the record. but who is recordingLOL)",
        R"(@flags=;pinned-chat-paid-amount=8761;turbo=0;user-id=35669184;pinned-chat-paid-level=ONE;user-type=;pinned-chat-paid-canonical-amount=8761;badge-info=subscriber/2;badges=subscriber/2,sub-gifter/1;emotes=;pinned-chat-paid-exponent=2;subscriber=1;mod=0;room-id=36340781;returning-chatter=0;id=289b614d-1837-4cff-ac22-ce33a9735323;first-msg=0;tmi-sent-ts=1687631719188;color=#00FF7F;pinned-chat-paid-currency=RUB;display-name=Danis;pinned-chat-paid-is-system-message=0 :danis!danis@danis.tmi.twitch.tv PRIVMSG #pajlada :-1 lulw)",
        // level 2
        R"(@room-id=36340781;tmi-sent-ts=1687970634371;flags=;id=39a80a3d-c16e-420f-9bbb-faba4976a3bb;badges=subscriber/6,premium/1;emotes=;display-name=rickharrisoncoc;pinned-chat-paid-level=TWO;turbo=0;pinned-chat-paid-amount=500;pinned-chat-paid-is-system-message=0;color=#FF69B4;subscriber=1;user-type=;first-msg=0;pinned-chat-paid-currency=USD;pinned-chat-paid-canonical-amount=500;user-id=518404689;badge-info=subscriber/10;pinned-chat-paid-exponent=2;returning-chatter=0;mod=0 :rickharrisoncoc!rickharrisoncoc@rickharrisoncoc.tmi.twitch.tv PRIVMSG #pajlada :forsen please read my super chat. Please.)",
    };
    return list;
}

const QStringList &getSampleEmoteTestMessages()
{
    static QStringList list{
        R"(@badge-info=subscriber/3;badges=subscriber/3;color=#0000FF;display-name=Linkoping;emotes=25:40-44;flags=17-26:S.6;id=744f9c58-b180-4f46-bd9e-b515b5ef75c1;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1566335866017;turbo=0;user-id=91673457;user-type= :linkoping!linkoping@linkoping.tmi.twitch.tv PRIVMSG #pajlada :Då kan du begära skadestånd och förtal Kappa)",
        R"(@badge-info=subscriber/1;badges=subscriber/0;color=;display-name=jhoelsc;emotes=301683486:46-58,60-72,74-86/301683544:88-100;flags=0-4:S.6;id=1f1afcdd-d94c-4699-b35f-d214deb1e11a;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1588640587462;turbo=0;user-id=505763008;user-type= :jhoelsc!jhoelsc@jhoelsc.tmi.twitch.tv PRIVMSG #pajlada :pensé que no habría directo que bueno que si staryuukiLove staryuukiLove staryuukiLove staryuukiBits)",
        R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=41:6-13,16-23;flags=;id=97c28382-e8d2-45a0-bb5d-2305fc4ef139;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922036771;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kreygasm, Kreygasm)",
        R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=25:24-28/41:6-13,15-22;flags=;id=5a36536b-a952-43f7-9c41-88c829371b7a;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922039721;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kreygasm,Kreygasm Kappa (no space then space))",
        R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=25:6-10/1902:12-16/88:18-25;flags=;id=aed9e67e-f8cd-493e-aa6b-da054edd7292;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922042881;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kappa.Keepo.PogChamp.extratext (3 emotes with extra text))",
        R"(@badge-info=;badges=moderator/1,partner/1;color=#5B99FF;display-name=StreamElements;emotes=86:30-39/822112:73-79;flags=22-27:S.5;id=03c3eec9-afd1-4858-a2e0-fccbf6ad8d1a;mod=1;room-id=11148817;subscriber=0;tmi-sent-ts=1588638345928;turbo=0;user-id=100135110;user-type=mod :streamelements!streamelements@streamelements.tmi.twitch.tv PRIVMSG #pajlada :╔ACTION A LOJA AINDA NÃO ESTÁ PRONTA BibleThump , AGUARDE... NOVIDADES EM BREVE FortOne╔)",
        R"(@badge-info=subscriber/20;badges=moderator/1,subscriber/12;color=#19E6E6;display-name=randers;emotes=25:39-43;flags=;id=3ea97f01-abb2-4acf-bdb8-f52e79cd0324;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1588837097115;turbo=0;user-id=40286300;user-type=mod :randers!randers@randers.tmi.twitch.tv PRIVMSG #pajlada :Då kan du begära skadestånd och förtal Kappa)",
        R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=41:6-13,15-22;flags=;id=a3196c7e-be4c-4b49-9c5a-8b8302b50c2a;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922213730;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kreygasm,Kreygasm (no space))",
        R"(@client-nonce=9e118ff9b63fbbeea66520f37929685d;source-id=040d1673-826d-48d1-9728-badc945e4a5e;user-type=staff;user-id=612865661;turbo=0;id=3ece90ea-9aaa-4634-bf3a-0105185da505;tmi-sent-ts=1727304403389;color=#DAA520;source-room-id=1025594235;flags=;emotes=emotesv2_8811dd848a214cef8a77575476cc33f4:0-9;subscriber=0;mod=0;emote-only=1;badges=staff/1,raging-wolf-helm/1;room-id=11148817;display-name=lahoooo;badge-info=;source-badge-info=;source-badges=staff/1,moderator/1,bits-leader/3 :lahoooo!lahoooo@lahoooo.tmi.twitch.tv PRIVMSG #pajlada shared9Dog)",
    };
    return list;
}

/// Channel point reward tests

const QString &getSampleChannelRewardMessage()
{
    static QString str{
        R"({"type":"MESSAGE","data":{"topic":"community-points-channel-v1.11148817","message":"{\"type\":\"reward-redeemed\",\"data\":{\"timestamp\":\"2022-11-19T13:36:49.536938653Z\",\"redemption\":{\"id\":\"fd8af65d-3532-4e91-b30e-3995cefe576b\",\"user\":{\"id\":\"11148817\",\"login\":\"pajlada\",\"display_name\":\"pajlada\"},\"channel_id\":\"11148817\",\"redeemed_at\":\"2022-11-19T13:36:49.536938653Z\",\"reward\":{\"id\":\"44c3554e-58bc-4753-bf94-b615931d1072\",\"channel_id\":\"11148817\",\"title\":\"Test\",\"prompt\":\"\",\"cost\":1,\"is_user_input_required\":false,\"is_sub_only\":false,\"image\":{\"url_1x\":\"https://static-cdn.jtvnw.net/custom-reward-images/11148817/44c3554e-58bc-4753-bf94-b615931d1072/effc017c-861d-410d-abff-8bd98f1937eb/custom-1.png\",\"url_2x\":\"https://static-cdn.jtvnw.net/custom-reward-images/11148817/44c3554e-58bc-4753-bf94-b615931d1072/effc017c-861d-410d-abff-8bd98f1937eb/custom-2.png\",\"url_4x\":\"https://static-cdn.jtvnw.net/custom-reward-images/11148817/44c3554e-58bc-4753-bf94-b615931d1072/effc017c-861d-410d-abff-8bd98f1937eb/custom-4.png\"},\"default_image\":{\"url_1x\":\"https://static-cdn.jtvnw.net/custom-reward-images/default-1.png\",\"url_2x\":\"https://static-cdn.jtvnw.net/custom-reward-images/default-2.png\",\"url_4x\":\"https://static-cdn.jtvnw.net/custom-reward-images/default-4.png\"},\"background_color\":\"#9147FF\",\"is_enabled\":true,\"is_paused\":false,\"is_in_stock\":true,\"max_per_stream\":{\"is_enabled\":false,\"max_per_stream\":0},\"should_redemptions_skip_request_queue\":false,\"template_id\":null,\"updated_for_indicator_at\":\"2021-03-16T00:40:29.385327044Z\",\"max_per_user_per_stream\":{\"is_enabled\":false,\"max_per_user_per_stream\":0},\"global_cooldown\":{\"is_enabled\":false,\"global_cooldown_seconds\":0},\"redemptions_redeemed_current_stream\":null,\"cooldown_expires_at\":null},\"status\":\"UNFULFILLED\",\"cursor\":\"ZmQ4YWY2NWQtMzUzMi00ZTkxLWIzMGUtMzk5NWNlZmU1NzZiX18yMDIyLTExLTE5VDEzOjM2OjQ5LjUzNjkzODY1M1o=\"}}}"}})",
    };
    return str;
}

const QString &getSampleChannelRewardMessage2()
{
    static QString str{
        R"({"type":"MESSAGE","data":{"topic":"community-points-channel-v1.11148817","message":"{\"type\":\"reward-redeemed\",\"data\":{\"timestamp\":\"2022-11-19T13:37:55.860377616Z\",\"redemption\":{\"id\":\"e8712d29-7564-40e9-a972-d70997665605\",\"user\":{\"id\":\"11148817\",\"login\":\"pajlada\",\"display_name\":\"pajlada\"},\"channel_id\":\"11148817\",\"redeemed_at\":\"2022-11-19T13:37:55.860377616Z\",\"reward\":{\"id\":\"649ecb1f-3fa8-491e-a48d-bd50720929d9\",\"channel_id\":\"11148817\",\"title\":\"asdd\",\"prompt\":\"asd\",\"cost\":5,\"is_user_input_required\":true,\"is_sub_only\":false,\"image\":null,\"default_image\":{\"url_1x\":\"https://static-cdn.jtvnw.net/custom-reward-images/default-1.png\",\"url_2x\":\"https://static-cdn.jtvnw.net/custom-reward-images/default-2.png\",\"url_4x\":\"https://static-cdn.jtvnw.net/custom-reward-images/default-4.png\"},\"background_color\":\"#0013A3\",\"is_enabled\":true,\"is_paused\":false,\"is_in_stock\":true,\"max_per_stream\":{\"is_enabled\":false,\"max_per_stream\":0},\"should_redemptions_skip_request_queue\":false,\"template_id\":null,\"updated_for_indicator_at\":\"2021-03-16T00:45:25.236177469Z\",\"max_per_user_per_stream\":{\"is_enabled\":false,\"max_per_user_per_stream\":0},\"global_cooldown\":{\"is_enabled\":false,\"global_cooldown_seconds\":0},\"redemptions_redeemed_current_stream\":null,\"cooldown_expires_at\":null},\"user_input\":\"TEST\",\"status\":\"UNFULFILLED\",\"cursor\":\"ZTg3MTJkMjktNzU2NC00MGU5LWE5NzItZDcwOTk3NjY1NjA1X18yMDIyLTExLTE5VDEzOjM3OjU1Ljg2MDM3NzYxNlo=\"}}}"}})",
    };
    return str;
}

const QString &getSampleChannelRewardIRCMessage()
{
    static QString str{
        R"(@badge-info=subscriber/43;badges=subscriber/42;color=#1E90FF;custom-reward-id=313969fe-cc9f-4a0a-83c6-172acbd96957;display-name=Cranken1337;emotes=;flags=;id=3cee3f27-a1d0-44d1-a606-722cebdad08b;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1594756484132;turbo=0;user-id=91800084;user-type= :cranken1337!cranken1337@cranken1337.tmi.twitch.tv PRIVMSG #pajlada :wow, amazing reward)",
    };
    return str;
};

/// Links

const QStringList &getSampleLinkMessages()
{
    static QStringList validLinks{
        R"(http://github.com/)",
        R"(https://github.com/)",
        R"(http://username@github.com/)",
        R"(https://username@github.com/)",
        R"(http://pajlada.github.io)",
        R"(https://pajlada.github.io)",
        R"(http://pajlada.github.io/)",
        R"(https://pajlada.github.io/)",
        R"(http://github.com/some/random/path)",
        R"(https://github.com/some/random/path)",
        R"(http://github.com?query=value)",
        R"(https://github.com?query=value)",
        R"(http://github.com?query=value&abc=123)",
        R"(https://github.com?query=value&abc=123)",
        R"(http://github.com/?query=value&abc=123&yhf=abc_def)",
        R"(http://github.com/?query=value&abc=123&yhf)",
        R"(http://github.com?query=value&abc=)",
        R"(http://github.com?query=value&abc=)",
        R"(http://github.com/#block)",
        R"(https://github.com/#anchor)",
        R"(http://github.com/path/?qs=true#block)",
        R"(https://github.com/path/?qs=true#anchor)",
        R"(github.com/)",
        R"(username@github.com/)",
        R"(pajlada.github.io)",
        R"(pajlada.github.io/)",
        R"(github.com/some/random/path)",
        R"(github.com?query=value)",
        R"(github.com?query=value&abc=123)",
        R"(github.com/?query=value&abc=123&yhf=abc_def)",
        R"(github.com/?query=value&abc=123&yhf)",
        R"(github.com?query=value&abc=)",
        R"(github.com?query=value&abc=)",
        R"(github.com/#block)",
        R"(github.com/path/?qs=true#block)",
        R"(HTTP://GITHUB.COM/)",
        R"(HTTPS://GITHUB.COM/)",
        R"(HTTP://USERNAME@GITHUB.COM/)",
        R"(HTTPS://USERNAME@GITHUB.COM/)",
        R"(HTTP://PAJLADA.GITHUB.IO)",
        R"(HTTPS://PAJLADA.GITHUB.IO)",
        R"(HTTP://PAJLADA.GITHUB.IO/)",
        R"(HTTPS://PAJLADA.GITHUB.IO/)",
        R"(HTTP://GITHUB.COM/SOME/RANDOM/PATH)",
        R"(HTTPS://GITHUB.COM/SOME/RANDOM/PATH)",
        R"(HTTP://GITHUB.COM?QUERY=VALUE)",
        R"(HTTPS://GITHUB.COM?QUERY=VALUE)",
        R"(HTTP://GITHUB.COM?QUERY=VALUE&ABC=123)",
        R"(HTTPS://GITHUB.COM?QUERY=VALUE&ABC=123)",
        R"(HTTP://GITHUB.COM/?QUERY=VALUE&ABC=123&YHF=ABC_DEF)",
        R"(HTTP://GITHUB.COM/?QUERY=VALUE&ABC=123&YHF)",
        R"(HTTP://GITHUB.COM?QUERY=VALUE&ABC=)",
        R"(HTTP://GITHUB.COM?QUERY=VALUE&ABC=)",
        R"(HTTP://GITHUB.COM/#BLOCK)",
        R"(HTTPS://GITHUB.COM/#ANCHOR)",
        R"(HTTP://GITHUB.COM/PATH/?QS=TRUE#BLOCK)",
        R"(HTTPS://GITHUB.COM/PATH/?QS=TRUE#ANCHOR)",
        R"(GITHUB.COM/)",
        R"(USERNAME@GITHUB.COM/)",
        R"(PAJLADA.GITHUB.IO)",
        R"(PAJLADA.GITHUB.IO/)",
        R"(GITHUB.COM/SOME/RANDOM/PATH)",
        R"(GITHUB.COM?QUERY=VALUE)",
        R"(GITHUB.COM?QUERY=VALUE&ABC=123)",
        R"(GITHUB.COM/?QUERY=VALUE&ABC=123&YHF=ABC_DEF)",
        R"(GITHUB.COM/?QUERY=VALUE&ABC=123&YHF)",
        R"(GITHUB.COM?QUERY=VALUE&ABC=)",
        R"(GITHUB.COM?QUERY=VALUE&ABC=)",
        R"(GITHUB.COM/#BLOCK)",
        R"(GITHUB.COM/PATH/?QS=TRUE#BLOCK)",
        R"(http://foo.com/blah_blah)",
        R"(http://foo.com/blah_blah/)",
        R"(http://foo.com/blah_blah_(wikipedia))",
        R"(http://foo.com/blah_blah_(wikipedia)_(again))",
        R"(http://www.example.com/wpstyle/?p=364)",
        R"(https://www.example.com/foo/?bar=baz&inga=42&quux)",
        R"(http://✪df.ws/123)",
        R"(http://userid@example.com)",
        R"(http://userid@example.com/)",
        R"(http://userid@example.com:8080)",
        R"(http://userid@example.com:8080/)",
        R"(http://142.42.1.1/)",
        R"(http://142.42.1.1:8080/)",
        R"(http://➡.ws/䨹)",
        R"(http://⌘.ws)",
        R"(http://⌘.ws/)",
        R"(http://foo.com/blah_(wikipedia)#cite-1)",
        R"(http://foo.com/blah_(wikipedia)_blah#cite-1)",
        R"(http://foo.com/unicode_(✪)_in_parens)",
        R"(http://foo.com/(something)?after=parens)",
        R"(http://☺.damowmow.com/)",
        R"(http://code.google.com/events/#&product=browser)",
        R"(http://j.mp)",
        R"(ftp://foo.bar/baz)",
        R"(http://foo.bar/?q=Test%20URL-encoded%20stuff)",
        R"(http://مثال.إختبار)",
        R"(http://例子.测试)",
        R"(http://उदाहरण.परीक्षा)",
        R"(http://-.~_!$&'()*+,;=:%40:80%2f::::::@example.com)",
        R"(http://1337.net)",
        R"(http://a.b-c.de)",
        R"(http://223.255.255.254)",
    };

    static QStringList validButIgnoredLinks{
        R"(http://username:password@github.com/)",
        R"(https://username:password@github.com/)",
        R"(http://userid:password@example.com)",
        R"(http://userid:password@example.com/)",
        R"(http://userid:password@example.com:8080)",
        R"(http://userid:password@example.com:8080/)",
    };

    static QStringList invalidLinks{
        R"(1.40)",
        R"(test..)",
        R"(test.)",
        R"(http://)",
        R"(http://.)",
        R"(http://..)",
        R"(http://../)",
        R"(http://?)",
        R"(http://??)",
        R"(http://??/)",
        R"(http://#)",
        R"(http://##)",
        R"(http://##/)",
        R"(http://foo.bar?q=Spaces should be encoded)",
        R"(//)",
        R"(//a)",
        R"(///a)",
        R"(///)",
        R"(http:///a)",
        R"(foo.com)",
        R"(rdar://1234)",
        R"(h://test)",
        R"(http:// shouldfail.com)",
        R"(:// should fail)",
        R"(http://foo.bar/foo(bar)baz quux)",
        R"(ftps://foo.bar/)",
        R"(http://-error-.invalid/)",
        R"(http://a.b--c.de/)",
        R"(http://-a.b.co)",
        R"(http://a.b-.co)",
        R"(http://0.0.0.0)",
        R"(http://10.1.1.0)",
        R"(http://10.1.1.255)",
        R"(http://224.1.1.1)",
        R"(http://1.1.1.1.1)",
        R"(http://123.123.123)",
        R"(http://3628126748)",
        R"(http://.www.foo.bar/)",
        R"(http://www.foo.bar./)",
        R"(http://.www.foo.bar./)",
        R"(http://10.1.1.1)",
    };

    static QStringList linkList{
        R"(@badge-info=subscriber/48;badges=broadcaster/1,subscriber/36,partner/1;color=#CC44FF;display-name=pajlada;emotes=;flags=;id=3c23cf3c-0864-4699-a76b-089350141147;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1577628844607;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada : Links that should pass: )" +
            validLinks.join(' '),
        R"(@badge-info=subscriber/48;badges=broadcaster/1,subscriber/36,partner/1;color=#CC44FF;display-name=pajlada;emotes=;flags=;id=3c23cf3c-0864-4699-a76b-089350141147;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1577628844607;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada : Links that should NOT pass: )" +
            validButIgnoredLinks.join(' '),
        R"(@badge-info=subscriber/48;badges=broadcaster/1,subscriber/36,partner/1;color=#CC44FF;display-name=pajlada;emotes=;flags=;id=3c23cf3c-0864-4699-a76b-089350141147;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1577628844607;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada : Links that should technically pass but we choose not to parse them: )" +
            invalidLinks.join(' '),
    };

    return linkList;
};

}  // namespace chatterino
