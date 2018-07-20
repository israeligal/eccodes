/*
 * Copyright 2005-2018 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "grib_api.h"

/*
 * Check that encoding of sub-truncation using IEEE64 works.
 * philippe.marguinaud@meteo.fr, 2016/02
 */

static double values[] = {
        2.78565217180879301395e+02,      0.00000000000000000000e+00,      8.38367712240454920902e-01,      0.00000000000000000000e+00,     -7.22151022768738215518e-01,
        0.00000000000000000000e+00,      1.54447137639483056404e+00,      0.00000000000000000000e+00,     -8.10351659720698336287e-01,      0.00000000000000000000e+00,
        6.45930713064333694717e-01,      0.00000000000000000000e+00,     -1.73417955255619204991e-01,      0.00000000000000000000e+00,     -9.09351867483918913093e-02,
        0.00000000000000000000e+00,     -3.00845517613623769648e-02,      0.00000000000000000000e+00,     -1.97522108657794209785e-01,      0.00000000000000000000e+00,
        1.45006392199800160370e-01,      0.00000000000000000000e+00,     -6.92319525648382033678e-02,      0.00000000000000000000e+00,     -3.37495245995370396486e-01,
        0.00000000000000000000e+00,      2.93969252845082296144e-01,      0.00000000000000000000e+00,      5.22791676917648828748e-02,      0.00000000000000000000e+00,
        -4.31020055899014042922e-02,      0.00000000000000000000e+00,     -5.34277055570271386387e-02,      0.00000000000000000000e+00,      1.57725778753141337996e-01,
        0.00000000000000000000e+00,     -1.38048966071163903280e-01,      0.00000000000000000000e+00,      7.95936533884674390915e-02,      0.00000000000000000000e+00,
        -5.08988096408558021722e-03,      0.00000000000000000000e+00,     -3.17356690459940984916e-02,      0.00000000000000000000e+00,     -7.97854100625838141836e-02,
        0.00000000000000000000e+00,     -1.25759522048120825355e-01,      0.00000000000000000000e+00,      1.41095505007377608475e-01,      0.00000000000000000000e+00,
        -2.90041709701537933630e-02,      0.00000000000000000000e+00,      1.16118748810570035501e-01,      0.00000000000000000000e+00,     -1.42650207377574778089e-01,
        0.00000000000000000000e+00,      6.63859656709936268859e-02,      0.00000000000000000000e+00,     -8.43008351127146754456e-02,      0.00000000000000000000e+00,
        8.11805506119530340126e-02,      0.00000000000000000000e+00,     -2.92579106275835476580e+00,     -2.73257362252726822283e-02,     -2.31503061096297768628e+00,
        2.98481883110496337697e-01,      2.32422954464165210098e+00,     -9.36624989809203695179e-02,     -1.13647173814196378316e+00,     -4.24970312443540054748e-01,
        1.26754821773852066613e+00,     -7.20859094680278700285e-02,     -9.51306475614490221204e-01,      6.14867632012408793507e-01,     -9.85091038732955892598e-02,
        -6.34523215803268758606e-02,     -6.78708060411115782218e-02,     -1.35311523427619539994e-01,     -9.04935301775932621204e-02,     -9.78101007785831533248e-02,
        -9.81097374871810895547e-04,      1.80616098092357818539e-02,     -2.64791786212412172397e-02,     -4.76558495508507054539e-02,     -1.96341419265033012564e-01,
        3.40906457596743608929e-02,     -7.81262194523913988675e-02,     -1.90201799489863598858e-01,      2.25178750048809411810e-01,      2.86704542482240504198e-03,
        -1.19805102746233238520e-02,      1.68969022321052431135e-02,      5.64738950483129331648e-02,     -1.39757566954298140249e-01,     -7.44084433787887133960e-02,
        -1.79448737058076172868e-02,      6.14333363921325120716e-02,      4.42090839521651901567e-02,     -1.59177621940344016238e-02,     -2.45259140787589882682e-02,
        4.14722887926062475095e-02,     -9.08467249754976585718e-02,      7.32634132869659504550e-02,      3.98083190502361433660e-02,     -6.23330309466623433412e-02,
        3.23061735747387016038e-02,      5.24308078374175548508e-02,     -6.52853670335678676340e-02,     -4.74582205706168894221e-02,     -2.57646217050176394581e-02,
        4.85326601320503064896e-02,      4.57836316487753580695e-02,     -1.61395013977115135695e-02,     -4.31399408593192076888e-02,     -1.72982609465274252136e-02,
        2.05141743795376405024e-02,      5.13479785723864323721e-02,      2.02753965345298277356e-02,     -9.62325942856205422682e-03,     -1.21013000039295196344e-03,
        4.61377663687331032727e-02,     -6.90153405245983028390e-04,     -1.66104804699747843344e+00,      1.79666906865138087390e-01,      7.23133875761683864170e-01,
        4.15103729262486009777e-01,     -3.04359433047895144853e-01,     -1.33601223735748869670e-01,      1.11896993727331572899e+00,     -5.97930191729183602156e-01,
        -3.21482407719980045613e-01,     -6.44440729482515239912e-02,     -1.39643167588630684461e-01,     -7.87344252201049310536e-02,     -2.74194962741515224636e-01,
        3.16014987883590156570e-02,      2.61916687188788577245e-01,     -2.29041295889389484586e-01,      1.37413993896893793512e-02,      1.02848928276271345261e-01,
        -8.45943398257167356036e-02,     -8.55836430183285895579e-02,     -2.14692485939022847941e-02,     -5.00660400534810728912e-02,      4.10286932308298671335e-02,
        5.98356967342531728682e-02,     -6.87268417209678655899e-02,     -1.00834342061879356223e-01,      2.89946855441678558174e-02,      6.84096422422015987275e-02,
        7.90905646811665974383e-02,      1.22761627402924594632e-02,     -7.19296897992387496146e-02,     -1.19563518571031840443e-01,      3.53949764125303933948e-02,
        3.49389806984224948749e-02,     -1.34556768717593006057e-01,      1.30036738800839157815e-01,      1.47453022488737178763e-01,     -1.75187106897877128975e-02,
        -5.71655857598896560701e-02,     -2.22513683371899778640e-02,      5.11984135243400070858e-02,      4.32198440146361120839e-02,     -3.79006811678369512353e-02,
        -1.61948869817943215399e-02,      5.87864730303968852088e-02,      7.56143666040565154418e-03,      3.79735451815043784096e-02,      4.88357571184367539319e-03,
        -3.44600211423619568266e-03,      8.45175251030964858057e-03,      2.06501052974025246789e-02,     -2.27945980601942863042e-02,      7.52792623890808781928e-02,
        -4.07335076464383702421e-02,     -1.22373247319515162691e-03,      7.12818582736400585542e-03,     -2.08797409246280901707e-02,      2.35344435031375059930e-02,
        5.38553837448995376125e-02,      2.29385900837120154661e-01,     -3.60512186055045147359e-01,      4.09262094434219958483e-01,     -1.43204915316633707434e-01,
        1.76661646314910159017e-01,      1.59424540157772631765e-01,      1.19996653025061628117e-01,      9.74032138671445946176e-02,      8.01756516767066496065e-02,
        -4.22092435868048232450e-01,      3.42598707334977881089e-01,      7.26427774784764446192e-02,     -1.92869792680440410582e-04,     -9.14800737566894561770e-02,
        1.86904852575672325576e-01,      9.03785536307215725538e-02,     -8.70628859333670845899e-02,     -1.20357023393743836626e-01,      1.30276821268167974921e-01,
        -5.50418496727855255379e-02,     -1.39930929689677463479e-01,     -4.10997318303747902202e-02,      1.66027061444833590187e-01,     -3.31025873454900559922e-02,
        5.45967001368218574076e-02,     -1.86051192758042188702e-01,      3.57669212138109621213e-02,      1.96823682403928168494e-01,      1.12736638944959083330e-01,
        -1.18563060708320622272e-01,     -7.11264215849067293895e-02,     -3.33098969956449747487e-02,      4.88100702343404121986e-02,      2.04094083090545080905e-02,
        7.78285563663724117012e-02,     -4.14113893806803898268e-02,      1.26375672481289953730e-02,      1.38745116741378599068e-01,     -8.57558938152793923115e-03,
        -9.29289689333271246019e-02,     -8.27276771926104548260e-02,      5.84956615429012907748e-02,      2.29399633044502938156e-03,     -2.09824524198216422546e-03,
        -2.30538708704958079931e-02,      3.77132043821169771203e-02,      6.53337758882697045015e-02,     -3.06667712165429387494e-02,     -1.14650515581519504144e-02,
        1.46194883433694872849e-02,      3.46636747707448856712e-03,     -4.35546278336838635359e-02,      6.29051255528407793349e-02,      6.12210568933847180256e-03,
        2.07571937975545607602e-02,      3.35768305063385619214e-01,      7.46266949448459948613e-01,     -6.72463631746016698987e-02,     -1.94536700956528363360e-01,
        -2.75088750614828209118e-02,      1.42328699967799371384e-01,      3.35288125725375085739e-01,      1.29661987766008346412e-01,      2.90956042046801172107e-02,
        -2.05442280573112395770e-01,     -9.57252926974513779212e-02,     -9.10355774240597659386e-02,     -1.25784172798783761005e-01,      1.12460127036075691054e-01,
        9.44013754824246931197e-02,     -6.05135930061245497225e-02,      9.42673278665148578481e-02,      2.00615512602382500829e-01,     -5.92185875904504069323e-02,
        -2.28592246943101873313e-01,     -1.07412950490968162054e-01,      2.20172956055192214908e-01,      3.33294698669218958376e-01,     -4.44283179185198731642e-02,
        -1.01766394690480529994e-01,      1.17472184755037761805e-01,      1.04892483649878465179e-01,     -6.62468314716531808672e-02,     -2.45565086356498013531e-03,
        4.79072727765457034854e-02,     -7.97766729243851169251e-04,     -2.33714338875874692858e-02,     -2.56642787088907850523e-02,     -9.62514763421860042314e-02,
        3.99383399281938444858e-02,      2.88865653017884548015e-02,     -5.15182520475396393822e-02,      8.26498791523299636275e-02,     -2.98742841129583554249e-02,
        -8.28182978045203938011e-02,      3.14455026979475749105e-02,     -1.53345808562610789669e-03,     -3.62400610998830294274e-02,     -2.51728077975302867719e-02,
        -3.02379015962623703961e-02,      2.68910770206758423240e-02,     -8.22634968902352312070e-03,      3.66544933660152699728e-02,      1.32356634436242674713e-02,
        3.21880247207419244518e-03,     -1.97571016662111687001e-02,      5.93796945717029508310e-03,     -4.31413602924163308572e-02,     -2.72339719264069676785e-02,
        -1.87274725743593273286e-01,      1.39283127485721136551e-01,      4.74963034217010249272e-01,     -3.42073553333540392174e-01,      4.83885077044606606855e-02,
        -2.59068005795831224347e-01,     -2.13264680765766156956e-01,     -7.36008484860203593403e-02,     -8.13152420843461926081e-02,     -1.21927665578479005326e-02,
        1.79904544651649317411e-01,     -1.09959740517953624694e-01,     -2.22065490779929819443e-01,      7.14015849729179447047e-02,     -5.85031872787839662964e-03,
        9.72943249874637094976e-02,      1.58464145663301525513e-01,     -6.23660570620122936547e-02,     -2.05086286960385583145e-01,     -5.07929431274403847540e-02,
        4.21897204870116185327e-02,     -6.99935466206487028051e-02,     -6.71348953496629374804e-02,      1.65743074443523596972e-01,      8.56850642116062655163e-02,
        -1.19953810412321809631e-01,     -7.40173516101229334030e-02,      1.18865945768013535344e-02,     -7.71584669380707965924e-02,     -6.31283399388396088137e-02,
        -1.45882854372074655114e-01,      6.29307315713034526317e-02,      1.48912624660626508738e-02,     -6.72819324032104260080e-02,      3.22547499735358483841e-02,
        4.97387743825052491831e-02,     -5.43073271907661761504e-02,      1.52590958854153246893e-02,      2.92721489192738311066e-03,     -1.87663839736718872364e-02,
        -3.80050872053503069684e-03,      5.91273447672226934446e-02,     -2.83684585809016896404e-03,      1.70512827209476636181e-02,     -1.44259815516434488497e-02,
        -8.06597987912826830392e-02,      5.19954562777717940736e-02,      1.09504480128524081528e-02,      1.26439885486616093030e-02,     -2.34593647937779382559e-03,
        2.47413111762442641806e-02,     -6.13559850860111253429e-03,     -6.05626633742831349538e-02,     -1.98467175793774985859e-01,      3.18979113074946862838e-01,
        6.23356771079433893967e-02,      1.30138108749532965813e-01,     -1.05230501466137910116e-01,     -6.87754575227078057686e-02,     -4.22436887367944324811e-02,
        7.57926459563205562331e-02,      4.78504387573109452036e-02,     -2.22681497547184761854e-02,      8.46951897073950493722e-02,     -7.44317622658197293462e-03,
        -1.89157622212636281067e-02,     -6.19719693803440774271e-02,     -8.37644347918033160827e-03,      1.32514981682897603488e-01,     -1.09126775888557903116e-01,
        -6.47849222903969929055e-03,      4.65591560634076861991e-02,     -2.24234845462696341656e-01,      7.29582023256248463072e-02,      2.23965783883813690514e-03,
        -5.19349169152241198211e-03,      2.25395864383192284786e-02,      9.10694488527918760701e-02,      7.36945737152017443794e-02,     -2.16337158651918673447e-02,
        -5.35436418307164291308e-02,      2.40342905027485786995e-02,      4.81946656785084762142e-02,     -3.06521617296798450092e-02,     -4.07904977042986977009e-02,
        6.32948267919029999851e-02,      4.53192474361057834331e-02,     -2.29008122631283589365e-02,      1.62576382380966721897e-02,      3.82842631619570161750e-02,
        3.52346794652655739832e-02,     -3.95023093184828782976e-02,     -3.15659719925446974331e-02,      4.02431236080928178556e-02,      2.45684617558553607120e-02,
        -1.93194945947791144047e-02,     -2.48322169406506726430e-02,     -1.13621415403786386183e-02,     -3.31487361399904478798e-02,      1.68675563537488623633e-02,
        4.01325091519489432490e-02,      4.14338905584917469027e-02,      1.31273776950603299207e-01,      7.16276979245435152510e-03,      5.02985992698696121606e-02,
        1.78045915098635626483e-01,     -1.86314001159308406663e-01,     -1.35151434606926579285e-01,      1.69947309928447143346e-01,     -8.37791941109623933270e-02,
        2.41195207762586322220e-02,      1.26309574838379373718e-01,     -2.00571859613532021971e-01,     -1.63465438373802895988e-01,     -6.96363659600393730686e-02,
        1.14524330229191645403e-01,      7.12890316169484949960e-02,      5.80381889626466196397e-03,      4.93537868059431283907e-02,     -8.44257746020753430116e-02,
        -1.41929196365875275043e-02,      8.22387462453117029648e-02,     -8.50093260265970818157e-02,      2.71192346905003156543e-02,      9.03395551304754379496e-02,
        -1.71516448697893314490e-02,      8.18953521749132690166e-02,      1.11511604557867297777e-02,     -4.78373059022189212053e-03,     -9.53023452930306103070e-02,
        5.38686323249407891800e-02,     -3.84678611713600085431e-02,     -5.04713920668615947246e-03,      6.80811773653991481048e-02,     -3.33049215638338835799e-02,
        -5.28099294606356045589e-02,     -7.54699335274512245197e-02,     -5.40781758032556764126e-03,      8.29793590010352716257e-02,     -5.97312971039127629802e-02,
        -2.71427292677366080453e-02,     -6.36007393417800138968e-03,     -6.21102608067901958838e-03,      1.20982775517295207401e-02,     -1.17366629581995736420e-02,
        3.79477291991657597614e-02,      4.16172556173764890408e-02,     -4.15482630953573559007e-02,     -2.10178763811884848633e-02,      2.25384141584651191692e-02,
        3.26544906456272876483e-01,      1.14875244914279722730e-01,     -1.66631737648658934114e-01,     -6.36733988328790686401e-03,      1.05207112351735077027e-01,
        -2.56656713269180158932e-01,      6.60485459910778310855e-02,      4.17130045127815929362e-02,     -7.37791748174861372611e-02,      7.60523512741725499220e-02,
        -4.55959006761720783696e-02,     -2.36680526647207629953e-02,      1.10634117950077798254e-01,     -2.08651727388676766495e-02,     -7.64711683504649686327e-02,
        8.13422913278145920390e-03,      7.93342835009738511776e-02,     -1.36145811582776665727e-01,      3.71088391279384144372e-02,      3.21562962981391253781e-02,
        -1.14387894243613869039e-01,     -1.51631103622234392203e-03,      5.78217444722389575795e-02,     -4.71124806075210594836e-03,      1.51729749910356030707e-02,
        9.74258177618566034717e-02,     -1.10011849267590264279e-01,     -9.55723084321756233273e-02,      5.40108889439728720450e-02,      2.20372759806924181647e-02,
        -3.12940070328804331723e-02,      7.31829279647891323135e-02,     -5.03897879848870661190e-02,      3.56857218059479508465e-02,      2.77447201596329011408e-02,
        -2.09552594775335998545e-02,      1.43951419283830060747e-02,      5.39902985778747748769e-02,      2.12233791683978864628e-02,     -1.06021857317053335573e-02,
        -1.29173575974317281917e-02,      2.33645540297061204277e-03,      2.61896609343018451146e-03,     -1.14595932568608803448e-02,      3.56767949738183843926e-04,
        2.01246788122515993247e-02,      1.99364141245472692443e-01,      1.02386975283006720350e-02,      1.46491539292862016364e-01,     -2.92173375533333390397e-01,
        -2.79882649001711690528e-02,      7.89598614348374083782e-02,      1.73932768517874730696e-02,     -1.69430993280276553925e-01,     -3.49439319604140236075e-02,
        -2.41776119023419042153e-02,     -1.19007451411664612329e-02,      7.52785183419538428407e-02,      8.00732324924654653708e-02,      5.80254140930885203842e-02,
        -2.99344525092792301812e-02,      2.00701049258613407889e-02,     -4.85173141225875570459e-02,     -1.24597145763424926868e-01,     -1.51901004582441958440e-02,
        -1.61680779553563594431e-02,      6.85542113027582250551e-02,      4.25745326736892623631e-02,     -1.06278911193231559440e-04,      9.76285197536541464458e-03,
        3.87428701891937912749e-02,     -2.30639988244256723127e-02,      6.36890026034679732764e-03,      3.02413182033626840028e-02,      3.88836672433736253024e-03,
        -9.08662711955680046927e-02,      4.01745378306784250988e-02,      1.03266559093635329480e-01,      2.14979137912293429002e-03,     -5.41722635351460576891e-02,
        2.61282597269434195553e-02,     -1.88195226737477086520e-03,     -2.19435523135619973967e-02,      1.32921056129775890658e-02,     -1.39584001825611234843e-02,
        -2.33277834487258597940e-02,      3.20387242392535485924e-03,     -2.56605748623718842027e-02,      3.00558911102263848214e-02,      3.05648112133436906934e-02,
        -2.59104772664633942192e-03,     -3.60234150814368395133e-02,      1.89051474579463585357e-01,      7.67659810167127809599e-02,     -1.24703387595890141659e-01,
        1.16494243349512860419e-01,     -1.23273291315800521267e-02,     -8.52472265397304362899e-02,      4.85995814815475199455e-02,     -4.91533864052588892468e-02,
        7.09392924651690154336e-02,      3.86918200904741022006e-02,     -6.05870726251201280332e-02,      1.26662290542081668043e-01,     -1.26300238339170367574e-02,
        -2.77371050405810226636e-02,     -5.43586513698262885352e-02,     -7.86459616541178574423e-02,      6.23528587727540037355e-02,     -4.79677851814970662714e-02,
        -6.51687150996678454806e-04,      5.76336385114276719688e-02,      1.44542478484825671209e-02,      3.16928492510423695516e-02,      1.45017405736592441823e-02,
        3.77649866255029068030e-02,      2.76488049642000443748e-04,     -6.60577998684190731415e-02,     -1.43760686140608738570e-02,      3.59108661833638931338e-02,
        -1.96544252969081192917e-02,     -3.14635605928988426605e-02,      2.73136448740384021883e-02,      4.81895944089887409700e-02,     -1.41798371596522010049e-03,
        -2.76940211432370950173e-02,     -1.79321359038478039816e-02,     -8.04857916575939247306e-03,      6.12601355103674924743e-04,     -3.04826514707566033346e-02,
        3.97737096905840994271e-03,      2.93070515299252015717e-02,     -1.68977509366934480761e-01,      3.32231696372570911580e-03,      1.25024830498748684704e-01,
        1.14603027997231002311e-01,     -1.00188353252315814901e-01,     -9.84663913699934667534e-03,      4.24578628782099620764e-02,     -1.37172078837733080192e-01,
        -3.49285758694722614504e-02,      5.10266827849673310080e-02,     -8.71727496405721724582e-02,      6.53026570180906262841e-03,      8.81977524226997683376e-02,
        -6.34979748614215300240e-02,     -6.75922576223946108831e-02,     -9.09058087393600777748e-03,      8.58354921744265697559e-02,     -3.11903783366274736655e-02,
        2.00028673474334506288e-02,      2.51992069666158204078e-02,     -1.15065874504069552003e-02,      1.90952708533619654185e-02,     -1.42994317615683774053e-02,
        8.38885300224805237346e-04,     -3.82027498742033497225e-02,     -3.67441895491770198490e-02,      2.50992455595355073994e-02,      7.01187400269996513602e-02,
        -5.54375280311450654991e-02,     -4.38519736099493434578e-02,     -7.84180536804670973161e-03,      2.15733075413469983783e-02,     -1.13430180042112986952e-03,
        1.43263625967320618049e-02,      3.38406892444173892920e-02,      2.61661740894084809961e-02,      1.89963433472952851477e-02,      4.11523810167366797808e-03,
        -4.07083694738235874616e-03,     -2.14250124636823323365e-02,     -2.51950730394823532721e-02,      3.42343547054715646727e-02,     -1.60995215305804545425e-01,
        1.46286861154573760713e-01,     -1.42470612032240401268e-02,     -8.74980046536548150549e-02,     -4.25453860188948623788e-02,      1.17182553786398857554e-01,
        -1.00991016453327253632e-01,     -7.91738577441953306213e-02,      5.29573588185536445194e-02,     -2.21614298461902045623e-02,      3.58602587208211864844e-02,
        2.52438926833398191718e-02,      9.41877819477729971709e-02,     -6.48116676539685077074e-02,     -6.92622200130770665494e-02,      3.44271533520238565140e-02,
        -2.14712430694437191037e-02,      1.72316382693014427674e-02,     -2.66608743584075963406e-02,     -2.10082338877249467923e-02,      8.91565305172099635544e-03,
        -1.23413533420058361967e-02,     -6.55699200671947673991e-03,     -6.50539642022832619028e-03,      1.51124007337151353147e-02,      5.20370287348198221067e-03,
        2.44544519960917354495e-02,      8.17735780033393942767e-03,     -3.77816692303542945967e-04,     -2.75580902253722351269e-02,      1.61366174765763085996e-02,
        -2.54225642602667507852e-02,     -5.68510449978476550825e-02,      1.03315681722852791002e-02,      5.48232041684815468024e-03,     -3.22722809472682503618e-02,
        1.68859338049149008965e-02,      6.21665123569529881609e-02,     -1.49734091796970530952e-01,     -1.69848035269845681894e-01,      7.28166844225807269320e-02,
        -9.38546660994197951000e-02,     -5.98972210309676594830e-02,      1.60189670628219316129e-01,      9.68228235109596380159e-02,     -1.14699218288179388692e-01,
        5.22495478003770982833e-02,     -2.50191304829539204468e-02,      2.77221049132821764638e-02,      5.25053174572039649259e-02,      4.67049795137407172674e-02,
        -4.25063623233455645245e-02,     -7.79015735544906322285e-03,      8.35061691171900405406e-02,     -2.77565926301914164820e-02,     -3.55176007501909543618e-02,
        5.22884040187019810686e-02,      4.47524306813492939866e-03,      3.34858736182796073999e-02,      2.18065090398272508343e-02,      3.96249948248765804237e-02,
        -1.75050000687652293485e-02,     -5.30639824787136182155e-03,      1.31658536276919365532e-02,     -3.54893637807925724226e-02,      1.44838563901157494143e-03,
        2.53219139441511024990e-02,      1.51209661740110620776e-02,     -3.18928979311501516886e-03,      8.08035892351891861518e-03,      1.49482343348299774599e-02,
        -4.67401524501743839690e-02,      8.81785937471118302922e-02,      1.09443858484468511949e-01,      9.61675669132599497679e-02,     -7.39592182705455791769e-02,
        8.87508822651789097691e-02,     -7.91714174638406542606e-02,     -1.25898861391198962911e-01,      8.84171948090256565322e-02,      3.93482215277929509023e-02,
        -6.26211797345710557972e-02,      3.11866892937116280127e-02,      1.54206070094080910371e-01,     -7.15962414998223273255e-02,      4.04187883182457047271e-02,
        3.87320351160306483584e-02,     -2.13087169719965857650e-02,      2.04352330201420621936e-02,      3.60324068311299108847e-02,     -6.71927151509099790383e-02,
        -5.86355402262732916530e-02,      1.95738272971190847838e-02,      1.19986570986449735121e-02,     -4.16517345466257682407e-02,      4.57582247311850084937e-02,
        -5.64919874467361762815e-02,     -2.96306742467898837426e-03,      3.45918894384134034681e-02,      9.37965919443191353810e-03,     -5.31266846947417263614e-03,
        7.44645101900219368030e-03,     -8.16779483872519251886e-03,     -1.49486275306286754694e-02,      5.40961754497425346966e-03,     -6.07743621110200162722e-03,
        -2.86027271239626297572e-02,      9.28607307596479303324e-02,      1.15334393912762608880e-01,      2.22619048233438435180e-02,     -2.81988516854455167904e-02,
        -3.27953711282213400402e-02,     -7.26623103447188861681e-02,     -6.21318073281297983401e-02,      8.15860644657136341307e-02,      1.79600966703069159225e-02,
        -8.26984672948943289050e-02,      2.66922837734803533594e-02,     -5.62012642483699781382e-02,     -7.54479658771655264937e-02,      5.63921972458279810447e-02,
        -8.88808665037604112014e-03,      4.49098738994229513752e-04,     -1.64624335502902702233e-02,      5.00902714527058645633e-02,      1.06477908341492902983e-02,
        2.19722684831073211686e-02,      9.27897245497274342219e-03,     -1.38071463629181757221e-02,     -3.25812324194547434097e-02,     -3.02235182235052954325e-02,
        2.70211283173303479233e-02,      1.78695725115783597692e-02,     -1.97856995310294429824e-02,      2.14583909244959528118e-02,      8.31160096115654556392e-03,
        -7.96044059263601583298e-03,     -7.03711411983747731069e-03,     -7.52245974116911853002e-02,     -7.17896739726268801052e-02,      1.07396425196690896431e-02,
        7.25906809169843403318e-02,     -9.45023972725838740105e-02,     -6.71199808984563744962e-02,      7.35264690791543329418e-02,      2.98610952212447697263e-02,
        -7.52442146662268135371e-04,      9.23976694955527116937e-02,      4.60854381027500578649e-02,     -5.81010137429742931636e-02,      5.19935015121687146844e-02,
        -4.28751313838296663206e-02,      2.68412731409498166169e-02,      9.92974085166531926505e-03,     -1.41649957602334279627e-02,      8.49484772510930227774e-03,
        1.60608334268962397659e-03,      1.53969582107269485627e-02,      2.53654424332110106644e-02,      2.20836817019017840613e-02,     -4.96400766026642691758e-03,
        -4.63895546537213768895e-03,     -3.69755978269468085773e-02,     -4.24220279369531480529e-02,     -2.80842055465305032516e-03,      3.43511446156298310903e-03,
        1.50427480682660263955e-02,     -7.64781824357823984684e-03,      1.55431405065905338392e-02,     -5.28685500234718219748e-02,     -4.81425552620864186104e-02,
        -6.98904426260522071424e-02,     -5.69063762150740712342e-02,     -2.01599484306710097226e-02,     -7.08142573082899164094e-03,      1.60387886663503793916e-02,
        -8.26785713680359968247e-02,      2.82003440014560874394e-02,      2.73932418290509169245e-02,     -3.17992788170617102228e-02,      5.85714331111230612770e-02,
        5.86577384557991701741e-02,     -2.75299012996250893476e-03,     -8.45195718330371027760e-03,     -1.36279044444791216234e-02,     -1.13870143130110085955e-02,
        -9.47170994530230694886e-03,      8.38589777991547574709e-03,     -2.44907795135516845320e-02,     -1.88112311627625997112e-02,      1.66825300517051550164e-02,
        -1.40472696715355817310e-02,      4.07602998308370448322e-02,      4.52331228574007898624e-03,     -7.41516905358174677892e-03,     -1.90839799893427405908e-02,
        1.37235319648593090402e-01,     -5.07166887609150802974e-03,      1.66347985382777688312e-02,     -8.94058332794331472870e-02,     -9.96617048317507969735e-03,
        -3.39998201752246195784e-02,     -2.72752305938337435975e-02,      1.41116199971288980557e-04,      1.20136294077674496417e-02,      1.21046096252801875515e-02,
        4.67374536088999582156e-03,      1.76093997807116745338e-03,     -6.49132489663982719524e-03,      6.34253853067380107822e-02,      4.20234078454048282092e-02,
        -3.39907942260764435460e-03,     -3.13684078013621997494e-02,     -5.59725664044557688626e-03,      1.46041344723689698465e-02,      8.15628093364801319554e-03,
        8.81123245682426982361e-03,     -5.36576704715815769320e-02,      4.13564181087699121731e-03,      6.66570175208870251832e-03,     -3.06753731552251258582e-03,
        3.24260047333155276839e-02,      3.78357123622976698057e-03,      1.22559252382635042178e-01,     -3.26726534731656623189e-02,     -4.57263244390499479231e-02,
        1.23161267442891183416e-02,      5.60898223233850107272e-02,     -2.07946281596515653023e-02,      3.30973059009364162231e-02,      3.79099335956614089116e-02,
        3.02099801034241572523e-03,     -4.11260349557933183040e-02,     -2.09662321613402133358e-02,     -1.35059512400454856396e-02,      2.11033519015176411482e-02,
        2.49738730383721862366e-03,      1.80042474049896738739e-03,     -4.21972590754290111104e-02,      7.84304881856809008034e-03,      1.28611420258033324476e-02,
        1.53304442551651341764e-02,      1.14809771240723442615e-02,      5.45508167269171534430e-03,      6.03486224308978115582e-03,     -6.17922023172545829134e-03,
        -1.06402780093771354153e-01,     -1.06566854283863088337e-01,     -1.56672555450536946098e-02,     -4.17617234129553371869e-02,     -5.84327485785076233826e-02,
        1.72707037153512245331e-02,     -4.27180361478920241763e-03,      3.83975587930814599158e-02,      7.18917519991502662613e-02,      1.73599075858622414748e-02,
        -5.06102136889801795805e-02,     -2.28302899957008066656e-02,      4.05449577890660689539e-02,      6.05903063932164507449e-03,      1.42495320779388468663e-02,
        -1.46505630681158707102e-02,     -5.51579872998721708333e-03,     -2.03950903125686006101e-02,      1.71600095189964607645e-02,      3.10141280680510215972e-02,
        -6.92078952483923121042e-03,     -1.25723580586239959064e-03,      4.83571917759556130223e-02,     -1.08848884748079619000e-01,     -7.62227492845969547414e-03,
        3.98614872899268266049e-02,      2.51496883852979809248e-02,     -7.45157236466782307849e-02,     -2.54315956467565489063e-02,      5.86785913495517369443e-02,
        6.07261945507166309849e-03,      2.37710782485035070077e-02,      2.40958099664499367232e-03,      8.00723308014901838381e-03,      5.57202821065413758661e-02,
        -6.18738592105032812751e-03,      1.57471692604664484198e-03,     -2.08797558382940772262e-03,     -2.73200765442419279960e-02,     -2.06208586891517380024e-02,
        6.40309150046379439887e-04,      3.34147637908005640250e-02,     -2.31245128603038314907e-02,      3.13805487530873783442e-02,     -3.76503956682762222496e-02,
        3.82033717675601958152e-02,      6.08549869979059313008e-02,     -1.40664950951139375995e-02,      1.20432450888018800772e-02,      4.11120541709728091334e-03,
        -1.31431111647770104689e-02,     -2.16767866450208908369e-02,      2.36444182462271527620e-03,      4.15518391649481646372e-03,     -7.19797119986096724242e-03,
        3.63099914737792378583e-02,     -1.71114207061806439247e-02,      9.12094809288829279359e-03,     -2.00581984724530065778e-02,      4.18093901030532240359e-03,
        -1.54353999486445108563e-02,      3.77632145097120189292e-02,     -8.67412239218371683425e-02,      3.74024476837445662980e-02,      7.02791955646850374784e-03,
        -3.07387087745127421390e-03,      1.26170642077405754933e-02,      7.19502473862098246987e-03,      5.00348237158451253359e-02,      6.05821676660315069918e-03,
        2.81657375913338843543e-02,     -2.75046494305255621871e-02,     -3.32847608680125889302e-02,      4.92458523340539092161e-03,     -6.97895182233836727098e-03,
        -1.84078432809154611538e-02,     -2.55254058040131799612e-02,      4.38596283726821068588e-03,      7.97305689082921688338e-03,     -6.65251068675700718558e-02,
        -2.43074168732615144076e-03,     -3.21551055785660854641e-03,     -2.14057386820096506863e-02,      1.73404829493880863200e-02,     -2.21788279126988834011e-02,
        3.67289476360874028726e-02,      2.88921423109056961509e-02,     -4.36590369050650264210e-03,     -3.38015966458028857197e-03,     -1.47872402615952351634e-02,
        -2.67937691397940741522e-02,     -4.01628891758317591032e-02,      5.37104159736995767638e-02,      3.38881421250558784175e-02,     -1.94731556323654853013e-02,
        6.80080451079642287447e-04,     -1.16224021866609874415e-02,     -2.84314312441163938305e-02,     -3.10403528556473483346e-02,     -1.52614844050795953750e-02,
        1.46865039960839046523e-02,      7.74337973212646264037e-03,     -2.75213939675030445919e-02,     -3.12476741289728875139e-02,     -3.75401476285800936306e-02,
        4.53067917901101985256e-02,      4.75497506720418041531e-03,      1.25627944556748021021e-02,      4.03046169919628882727e-02,     -3.29378978115561424356e-02,
        -2.51499548928088654393e-03,     -1.99564144518295516484e-02,     -2.63938123636600005528e-02,     -1.84979990476826575141e-02,     -4.75891539387540141171e-02,
        1.70189645456799328038e-02,      2.90621732209333551167e-02,      1.69695748997654785595e-02,      6.00578109434020054880e-03,      3.77405846672976205691e-02,
        1.04426223615697640751e-02,     -7.42237607930411715310e-04,     -1.51118572042552353624e-02,      1.29952623161545995079e-03,      1.26799047323642617713e-02,
        1.05278997248968164691e-02,      2.29682654055416574090e-02,      1.28888330591014361320e-02,     -1.03014730669756075787e-02,     -4.83421710205830541113e-03,
        -1.90040068302814585399e-02,     -4.31802671175361696410e-03
};

#define ILCHAM 992
#define MTRONC  30
#define STRONC  10

int main (int argc, char * argv[])
{
    size_t len;
    grib_handle * h;
    double zval[ILCHAM];
    int m, n, k;

    GRIB_CHECK (((h = grib_handle_new_from_samples (NULL, "sh_ml_grib2")) == NULL), 0);

    /* Meteo-France settings */
    GRIB_CHECK (grib_set_long (h, "centre", 85), 0);
    len = strlen ("stretched_rotated_sh");
    GRIB_CHECK (grib_set_string (h, "gridType", "stretched_rotated_sh", &len), 0);
    GRIB_CHECK (grib_set_long (h, "pentagonalResolutionParameterJ", MTRONC), 0);
    GRIB_CHECK (grib_set_long (h, "pentagonalResolutionParameterK", MTRONC), 0);
    GRIB_CHECK (grib_set_long (h, "pentagonalResolutionParameterM", MTRONC), 0);

    GRIB_CHECK (grib_set_double (h,"stretchingFactor", 2.40000000000000), 0);
    GRIB_CHECK (grib_set_double (h,"latitudeOfStretchingPoleInDegrees", 46.4688478326275), 0);
    GRIB_CHECK (grib_set_double (h,"longitudeOfStretchingPoleInDegrees", 2.57831007808864), 0);

    GRIB_CHECK (grib_set_long (h, "bitsPerValue", 16), 0);
    len = strlen ("spectral_complex");
    GRIB_CHECK (grib_set_string (h, "packingType", "spectral_complex", &len), 0);

    GRIB_CHECK (grib_set_long (h, "subSetJ", STRONC), 0);
    GRIB_CHECK (grib_set_long (h, "subSetK", STRONC), 0);
    GRIB_CHECK (grib_set_long (h, "subSetM", STRONC), 0);
    GRIB_CHECK (grib_set_long (h, "unpackedSubsetPrecision", 2), 0);

    GRIB_CHECK (grib_set_double_array (h, "values", values, ILCHAM), 0);
    len = ILCHAM;
    GRIB_CHECK (grib_get_double_array (h, "values", zval, &len), 0);

    for (m = 0, k = 0; m < MTRONC+1; m++)
        for (n = m; n < MTRONC+1; k++, n++)
        {
            /* Check sub-truncaton was fully preserved */
            if ((m < STRONC+1) && (n < STRONC+1) && ((zval[2*k] != values[2*k]) || (zval[2*k+1] != values[2*k+1])))
            {
                printf ("Unpacked sub-truncation was not fully preserved; coefficients for wavenumber (m=%d,n=%d) have been modified\n", m, n);
                return 1;
            }
        }

    GRIB_CHECK (grib_handle_delete (h), 0);

    return 0;
}
