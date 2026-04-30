import ui




def on_click (  )  : 
    print ( "Button clicked!" ) 



win   =    ui.Window ( "Example Python App" ,   100 ,   100 ,   400 ,   300 ) 
btn   =    ui.Button ( "Click Me" ,   50 ,   50 ,   100 ,   40 ,   on_click ) 
win.add ( btn ) 
win.show (  ) 
