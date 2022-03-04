تمرین گروهی ۱/۰ - آشنایی با pintos

======================

شماره گروه: 3

-----

> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

علی جعفری <alijafari13791397@gmail.com>

امید رستم آبادی <omidrostamabadi@gmail.com> 

سینا مظاهری <sinamazaheri1707@gmail.com> 

پیمان حاجی محمد <peyman.eb15@gmail.com> 

مقدمات

----------

> اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.

> لطفا در این قسمت تمامی منابعی (غیر از مستندات Pintos، اسلاید‌ها و دیگر منابع  درس) را که برای تمرین از آن‌ها استفاده کرده‌اید در این قسمت بنویسید.

+ https://en.wikibooks.org/wiki/X86_Assembly/X86_Architecture

+ https://www.eecg.utoronto.ca/~amza/www.mindsec.com/files/x86regs.html

+ https://en.wikipedia.org/wiki/X86_calling_conventions

+ 

آشنایی با pintos

============

>  در مستند تمرین گروهی ۱۹ سوال مطرح شده است. پاسخ آن ها را در زیر بنویسید.

## یافتن دستور معیوب

۱.

آدرس مجازی حافظه = 0xc0000008

۲.

آدرس مجازی دستور = 0x8048757

۳.

نام تابع = start_

دستور کرش کرده = mov    0x24(%esp),%eax

۴.

در فایلی در آدرس /home/vagrant/code/group/pintos/pintos/src/lib/user/entry.c تابع مورد نظر وجود دارد که در ادامه محتوای تابع آورده شده است:

    void _start (int argc, char *argv[]){

            exit (main (argc, argv));    

    }

0x08048754 <+0>:     sub    $0x1c,%esp
  0x08048757 <+3>:     mov    0x24(%esp),%eax
 0x0804875b <+7>:     mov    %eax,0x4(%esp)
  0x0804875f <+11>:    mov    0x20(%esp),%eax
 0x08048763 <+15>:    mov    %eax,(%esp)
0x08048766 <+18>:    call   0x80480a0 <main>
0x0804876b <+23>:    mov    %eax,(%esp)
 0x0804876e <+26>:    call   0x804a2bc <exit>

توضیحات هر خط در ادامه آورده شده است:
1. دستور sub استک پوینتر را کم  می کند تا به سمت پایین رفته و فضا در استک برای ذخیره سازی داده های بعدی ایجاد شود
2. این دستور مقدار argv را که در آدرس (استک پوینتر+ 0x24 ) است را در ثبات eax  ذخیره می کند
3. این دستور مقدار argv درون ثبات eax را درون خانه به آدرس (esp + 0x4) می ریزد
4. سپس از آدرس (esp + 0x20) مقدار argc را درون ثبات eax ذخیره می کند
5. حال مقدار argc ذخیره شده در ثبات eax را درون آدرس (esp) ذخیره می کند
6. حال تابع main فراخوانی می شود
7. و در ادامه مقدار return شده تابع main را که در ثبات eax ریخته شده است را درون آدرس esp میریزد
8. در نهایت تابع exit را فراخوانی می کند.

۵.
تابع start_ می خواهد آرگومان های argv و argc را از پشته خود برداشته و در آرگومان های تابع main قرار دهد. آدرس 0xc0000008 دارای محتوای argv می باشد که این دقیقا همان آدرسی است که در do-nothing.result به مشکل page fault بر خورد می کرد. 

## به سوی crash

۶.

    pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec <incomplete sequence \357>, priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0

    x0, magic = 3446325067} 

    pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priori    

نام ریسه در حال اجرا: main

آدرس ریسه: 0xc000e000

سایر ریسه های موجود: idle

۷.

خروجی backtrace:

    #0  process_execute (file_name=file_name@entry=0xc0007d50 "do-nothing") at ../../userprog/process.c:32

    #1  0xc0020268 in run_task (argv=0xc00357cc <argv+12>) at ../../threads/init.c:288#2  0xc0020921 in run_actions (argv=0xc00357cc <argv+12>) at ../../threads/init.c:340

    #3  main () at ../../threads/init.c:133 

کد فراخوانی c توابع مربوطه:

    process_wait (process_execute (task)); //init.c:288

    run_test (task); //init.c:290

    call main //Start.s:180

۸.

خروجی دستور dumplist:

    pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0037314 <temporary+4>, next = 0xc003731c <temporary+12>}, pagedir = 0x0, magic = 3446325067}

    pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

    pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

ریسه های موجود: main , idle , do-nothing

۹.

در کد process.c در تابع process_execute در خط شماره 45:

    tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);

۱۰.

    $1 = {edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code = 0x0, frame_pointer = 0x0, eip = 0x8048754, cs = 0x1b, eflags = 0x202, esp = 0xc0000000, ss = 0x23}

۱۱.
دستور ire،  مقادیر (ss:esp) eflags و (cs:eip) را از پشته pop می کند.
حالت پردازنده با دو بیت آخر (lsb) ثبات های  segment از جمله ثبات cs مشخص می شود که اگر 11 باشد در ring 3 هستیم و در واقع در usermode قرار داریم.
۱۲.

eax            0x0      0
ecx            0x0      0
edx            0x0      0
ebx            0x0      0
esp            0xc0000000       0xc0000000
ebp            0x0      0x0
esi            0x0      0
edi            0x0      0
eip            0x8048754        0x8048754
eflags         0x202    [ IF ]
cs             0x1b     27
ss             0x23     35
ds             0x23     35
es             0x23     35
fs             0x23     35
gs             0x23     35

همانگونه که قابل مشاهده است مقادیر رجیستر ها با IF_ تفاوتی نکرده است، علت این امر این است که دستور iret تنها مقادیر  درونی ساختار IF_ را که در استک قرار دارد درون رجیستر ها ذخیره می کند

۱۳.

    #0  _start (argc=<unavailable>, argv=<unavailable>) at ../../lib/user/entry.c:9

## دیباگ

۱۴.
در خط 445 در کد process.c داریم:

	*esp = PHYS_BASE //process.c:445

که این بدین معناست که user تلاش می کند در آدرس 0xc000008 مقدار argc را پوش کند که seg fault میخورد زیرا استک kernel تا آدرس 0xc000000 در عهده خود دارد و اجازه دسترسی این آدرس های حافظه به user داده نشده است بنابراین لازم است در خط کد بالا مقداری مناسب را از مقدار PHYS_BASE کم نمود تا برای  پوش کردن وارد آدرس حافظه ای شویم که برای kernel نبوده و اجازه دسترسی به آن داریم. برای مثال مقدار 0x14 را از مقدار آن نیز کم می کنیم
۱۵.
حال با توجه به استاندارد x86 ABI میدانیم که هنگامی که در کد اسمبلی به دستور call می رسیم، آدرس درونی esp  باقی مانده آن به 16 بایستی 0 باشد. همچنین خود دستور call نیز از esp چهار آدرس کم کرده بنابراین در main آدرس درونی esp بایستی رقم کم ارزش 12 داشته باشد 

	0xc0000000 − 0x14 = 0xbfffffec
در مرحله بعد کامپایلر مقدار 0x1c را از esp کم می کند که در نهایت در بخش قبل از دستور call آدرس ما باقی مانده به 16 برابر با صفر خواهد داشت.
۱۶.

	0xbfffffa8:     0x00000001      0x000000a2

۱۷.
args[0]=1
args[1]=162
این مقادیر همان مقادیر درون استک بوده اند که در اینجا به syscall handler پاس داده شده اند
۱۸.

	sema_down (&temporary);//process.c:94 (in function process_wait)
۱۹.
نام ریسه در حال اجرا: main
آدرس ریسه در حال اجرا: 0xc000e000
سایر ریسه ها:idle, do-nothing\000\000\000\000\000
	pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec "\375\003", priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
	pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
	pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_READY, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
