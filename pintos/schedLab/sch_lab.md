1.1 پیاده کردن شبیه ساز زمانبندی
آ)
خروجی مربوط به SRTF در زیر قابل مشاهده است:

    0: Arrival of Task 12 (ready queue length = 1)
    0: Run Task 12 for duration 2 (ready queue length = 0)
    1: Arrival of Task 13 (ready queue length = 1)
    2: Arrival of Task 14 (ready queue length = 2)
    2: IO wait for Task 12 for duration 1
    2: Run Task 14 for duration 1 (ready queue length = 1)
    3: Arrival of Task 15 (ready queue length = 2)
    3: Wakeup of Task 12 (ready queue length = 3)
    3: IO wait for Task 14 for duration 2
    3: Run Task 12 for duration 2 (ready queue length = 2)
    5: Wakeup of Task 14 (ready queue length = 3)
    5: Run Task 14 for duration 1 (ready queue length = 2)
    6: Run Task 15 for duration 2 (ready queue length = 1)
    8: Run Task 15 for duration 1 (ready queue length = 1)
    9: Run Task 13 for duration 2 (ready queue length = 0)
    11: Run Task 13 for duration 2 (ready queue length = 0)
    13: Run Task 13 for duration 2 (ready queue length = 0)
    15: Run Task 13 for duration 1 (ready queue length = 0)
    16: Stop

ب)
خروجی مربوط به بخش mlfq2 در بخش زیر آورده شده است:

	

    0: Arrival of Task 12 (ready queue length = 1)
    0: Run Task 12 for duration 2 (ready queue length = 0)
    1: Arrival of Task 13 (ready queue length = 1)
    2: Arrival of Task 14 (ready queue length = 2)
    2: IO wait for Task 12 for duration 1
    2: Run Task 13 for duration 2 (ready queue length = 1)
    3: Arrival of Task 15 (ready queue length = 2)
    3: Wakeup of Task 12 (ready queue length = 3)
    4: Run Task 14 for duration 1 (ready queue length = 3)
    5: IO wait for Task 14 for duration 2
    5: Run Task 15 for duration 2 (ready queue length = 2)
    7: Wakeup of Task 14 (ready queue length = 3)
    7: Run Task 12 for duration 2 (ready queue length = 3)
    9: Run Task 14 for duration 1 (ready queue length = 2)
    10: Run Task 13 for duration 4 (ready queue length = 1)
    14: Run Task 15 for duration 1 (ready queue length = 1)
    15: Run Task 13 for duration 1 (ready queue length = 0)
    16: Stop
	


1.2 آزمایش 2

آ)
چون X_i توزیع نمایی با پارامتر a دارد، و میدانیم که میانگین توزیع نمایی با این پارامتر برابر 1/a است، بنابراین میتوان a=1/M انتخاب کرد.

ب)
اگر طول هر فعالیت M باشد، در حالت ایده آل باید 50% زمان CPU استفاده شود. در یک CPU با c هسته، باید مقدار a در توزیع نمایی برابر c * 1/2M انتخاب شود. در اینصورت به صورت میانگین نصف وقت هر هسته به idle و نصف دیگر وقت به فعالیتهای مفید اختصاص خواهد یافت.
نتایج شبیه سازی در سلول مربوطه فایل notebook قابل مشاهده هستند.

د)
طبیعتاً با افزایش نرخ ورود فعالیتها بهره وری افزایش پیدا میکند تا به 100% برسد.

ه) 
با افزایش بهره CPU، زمان پاسخگویی فعالیتهای مختلف نیز افزایش میابد.

و)
بله، نوع زمان بند موثر است. میانه پاسخ دهی برای srtf از همه کمتر است.
در مجموع، برای fcfs تسکهای کوتاه با تاخیر زمان پاسخ دهی زیادی مواجه میشوند (چون ممکن است پشت تسکهای طولانی منتظر بمانند).
در round_robin هم بستگی به سایز کوانتوم دارد. اگر سایز به سمت بی نهایت برود به fcfs میل میکند. برای سایزهای کوچک، تسکهای با زمان اجرای کم تاخیر خوبی به دست می آورند. برای تاخیر خوب، بهتر است سایز کوانتوم را طوری بگیریم که اکثر تسک های interactive سیستم در یک یا دو کوانتوم بتوانند تمام شوند. در این صورت تاخیر پاسخ دهی این تسکها خیلی خوب میشود و برای تسکهای طولانی هم خیلی این تسکهای کوچک اثرگذار نیستند.
در srtf که حالت ایده آل را داریم. در حالت تساوی، باید به این دقت کنیم که زمان شروع چه تسکی زودتر بوده و اول آن را به اتمام برسانیم که تاخیر پاسخ دهی کمتری داشته باشیم.
همچنین mlfqs هم عملکرد خوبی دارد و در واقع با افزایش levelها سعی میشود این نوع زمان بند تقریب خوبی از srtf باشد که در واقعیت قابل پیاده سازی نیست.
ز)
 حالت ایده آل این است که CPU فقط به یک تسک اختصاص داشته باشد. در این صورت response time هر تسک دقیقاً برابر زمان اجرای تسک است.  وقتی سامانه ای بهره وری بالا دارد یعنی تسکهای زیادی وارد میشوند و باید سرویس دهی شوند و ممکن است تسک فعلی منتظر تسکهای دیگر بماند که کارشان تمام شود. هر چقدر تسکهای سیستم بیشتر باشند این احتمال انتظار بیشتر بوده و response time افزایش خواهد یافت.
 البته در شبیه سازی پارامترهای مهمی مثل هزینه context switch دیده نمیشوند، این موارد نیز باعث افزایش میشوند.

1.3 رعایت عدالت بین فعالیتها:

آ)
چون تابع burst در شبیه سازی هیچ گاه درخواست تسک جدیدی را تا تکمیل شدن تسک قبلی نمیدهد پس طول صف حداکثر 2 خواهد ماند.

ب)
چون توزیع S1 و T1 عیناً مشابه است، پس رخدادهای P[S1 < T1] و P[S1 > T1] هیچ فرقی با هم ندارند. از طرفی چون جمع آنها 1 است، پس احتمال هر کدام بایستی 1/2 باشد.

ج)
بر اساس قضیه حد مرکزی میدانیم:
S_m_bar = S / m
Limit (S_m_bar - S_bar) * sqrt(m) / sqrt(Var(s)) = Standard normal (as n approaches infinity)
بنابراین برای S خواهیم داشت:
S_m_bar = Normal (S_bar, Var(s) / m)
S = m * Normal (S_bar, Var(s) / m) = Normal (m * S_bar, m * Var(s))

د)
میتوانیم به جای CPU_timeها که همان متغیر S و T هستند، معادلاً S_m_bar و T_m_bar قرار دهیم چون تنها در یک ثابت اختلاف دارند.
فرض کنیم S_m_bar = X و T_m_bar = Y در این صورت خواهیم داشت:
X = Normal (u, sigma / m)
Y = Normal (u, sigma / m)
که در آن فرض شده u = E[Si] و sigma = Var[Si]. در این صورت احتمال خواسته شده به صورت زیر محاسبه میشود:
P[X - alpha * Y > 0] = P[Z > 0]
که در آن داریم:
Z = Normal ((1-alpha) * u, (1 + alpha^2) * sigma / m)
بنابر قسمت بالا، احتمال P[Z > 0] بر حسب تابع توزیع تجمعی نرمال به صورت زیر نوشته خواهد شد:
P[Z > 0] = F_Z (0) = 1 - Phi ( (0 - u_z) / sqrt (sigma_z) * sqrt(m) ) = 1 - Phi ( ( - u_z) / sqrt (sigma_z) * sqrt(m) )
u_z = E[Z] = (1 - alpha) * u
sigma_z = (1 + alpha^2) * sigma

ه)
اگر شرط تساوی گفته شده را لحاظ کنیم خواهیم داشت:
1 - Phi ( ( - u_z) / sqrt (sigma_z) ) = 1 - Phi ( sqrt(m) * (alpha - 1) / sqrt(1 + alpha^2))
در نهایت با جایگذاری مقادیر، برای m=100 خواهیم داشت:
1 - Phi (0.67367) = 25%
اما این فقط احتمال T > S است. چون حالت دیگر نیز عیناً مشابه است، پس در مجموع احتمال اینکه یکی حداقل 10% از دیگری بیشتر CPU مصرف کند برابر 50% خواهد بود!
برای m=10000 آرگومان داخل Phi ده برابر شده و احتمال به کمتر از یک صد میلیونییم درصد میرسد.
بنابراین در تعداد اجراهای کم ممکن است عدالت خیلی رعایت نشود که هیچ، با احتمال نسبتاً بالا یکی از تسک ها میتواند 10% بیشتر از CPU استفاده کند. نظر بهنام فقط وقتی درست است که m واقعاً بزرگ باشد.

و)
در این قسمت دو تسک را درست میکنیم. زمان انجام هر تسک را به اندازه جمع m متغیر رندوم (در اینجا نمایی را انتخاب کردیم با lambda=1.0) قرار داده میشود. سپس به اندازه TRIALS بار این کار تکرار شده و هر بار نسبت تعداد رانهای unfair به fair محاسبه میشود. برای m=100 همواره خیلی نزدیک به 50% برای نرخ N=1.1 غیرعادلانه بودن داریم. ولی برای m=10000 این مقدار به 0 میرسد.
نموداری در پایین دفترچه مربوط به این بخش قابل مشاهده است که نمودار احتمال استفاده کردن 10% بیشتر توسط یک تسک نسبت به تسک دیگر را بر حسب m رسم میکند. حدودی میتوان گفت برای اطمینان از fairness باید m > 2000 باشد.