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

۵.

## به سوی crash

۶.

۷.

۸.

۹.

۱۰.

۱۱.

۱۲.

۱۳.

## دیباگ

۱۴.

۱۵.

۱۶.

۱۷.

۱۸.

۱۹.