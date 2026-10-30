use crate::quadtranslator::QuadTranslator;
use std::os::raw::{c_char};
use oxigraph::store::Store;
use std::io::Error;
use crate::store2clips::store2clips;

pub struct ClipsSerializer{
    pub store: Store,
    translator: QuadTranslator,
}

impl ClipsSerializer {
    pub fn new() -> Result<Self, ()> {
        let store = match Store::new() {
            Ok(x) => x,
            Err(e) => {
                eprintln!("ERROR Oxigraph::Store::new failed: {:?}", e);
                return Err(());
            },
        };
        Ok(Self {
            store: store,
            translator: QuadTranslator::new(),
        })
    }

    pub fn finish(&mut self) -> Result<Vec<u8>, Error> {
        return store2clips(&self.store);
            /*
        use std::mem::replace;
        self.in_write_state();
        let writer = match replace(&mut self.writer, None) {
            None => {return Err(
                Error::other("TriGSerializer in wrong state to finish."));
            },
            Some(w) => w,
        };
        let mut ret = match writer.finish() {
            Ok(x) => x,
            Err(e) => {return Err(e);},
        };
        ret.push(0); //ensure trailing '\0'
        Ok(ret)
            */
    }

    pub fn add_triple(
        &mut self, 
        subject: *const c_char, subject_type: u8,
        predicate: *const c_char,
        object: *const c_char, object_suffix: *const c_char,
        object_type: u8,
    ) -> i8
    {
        let q_quad = self.translator.ctriple_to_quad(subject, subject_type,
                            predicate, object, object_suffix, object_type);
        let quad = match q_quad {
            Ok(x) => {x},
            Err(_) => {return -1;}
        };
        match self.store.insert(&quad) {
            Ok(_) => {return 0;},
            Err(_) => {
                eprintln!("ERROR ClipsSerializer.add(add quad to store)");
                return -3;
            }
        }
    }
}
