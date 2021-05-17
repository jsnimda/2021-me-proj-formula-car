import eel
eel.init('web')


@eel.expose
def send_text(s):
    print(s)
    eel.set_hist(s)


eel.start('client.html', size=(1600, 900), block=False)


print(eel.js_random()())

def my_other_thread():
    while True:
        print("I'm a thread")
        eel.sleep(1.0)                  # Use eel.sleep(), not time.sleep()

# eel.spawn(my_other_thread)

eel.set_hist({
    'test': [1,2,3,4],
    'nono': 456
})

while True:
    # print("I'm a main loop")
    eel.sleep(1.0)   
