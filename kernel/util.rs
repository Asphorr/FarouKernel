use std::{fmt, ops};

/// A unique pointer wrapper that supports both single objects and arrays.
#[derive(Debug)]
pub struct UniquePtr<T> {
    /// The wrapped value.
    inner: Option<Box<dyn DerefMut<Target = T>>>,

    /// Whether the wrapped value is an array or not.
    is_array: bool,
}

impl<T> UniquePtr<T> {
    /// Creates a new instance of `UniquePtr`.
    pub fn new(value: impl FnOnce() -> T) -> Self {
        Self {
            inner: Some(Box::new(value())),
            is_array: false,
        }
    }

    /// Moves the contents of another `UniquePtr` into this one.
    pub fn move_from(other: &Self) {
        *this = other.clone();
    }

    /// Returns a mutable reference to the wrapped value.
    pub fn get_mut(&mut self) -> &mut T {
        match self.inner {
            Some(ref mut inner) => inner.get_mut().unwrap(),
            None => panic!("Attempted to access moved-out UniquePtr"),
        }
    }
}

impl<T> Drop for UniquePtr<T> {
    fn drop(&mut self) {
        if !self.is_array && self.inner.is_some() {
            unsafe {
                let _ = Box::into_raw(self.inner.take().unwrap()).drop();
            }
        }
    }
}

impl<T> fmt::Display for UniquePtr<T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:p}", self.inner.as_ref().map(|x| x.as_ref()))?;
        Ok(())
    }
}

impl<T> Clone for UniquePtr<T> {
    fn clone(&self) -> Self {
        Self {
            inner: self.inner.clone(),
            is_array: self.is_array,
        }
    }
}

impl<T> Default for UniquePtr<T> {
    fn default() -> Self {
        Self::new(|| T::default())
    }
}

impl<T> From<Box<dyn DerefMut<Target = T>>> for UniquePtr<T> {
    fn from(boxed: Box<dyn DerefMut<Target = T>>) -> Self {
        Self {
            inner: Some(boxed),
            is_array: false,
        }
    }
}

impl<T> From<Vec<T>> for UniquePtr<T> {
    fn from(vec: Vec<T>) -> Self {
        Self {
            inner: Some(Box::new(vec)),
            is_array: true,
        }
    }
}

impl<T> AsRef<T> for UniquePtr<T> {
    fn as_ref(&self) -> &T {
        self.inner.as_ref().map(|x| x.as_ref()).unwrap()
    }
}

impl<T> AsMut<T> for UniquePtr<T> {
    fn as_mut(&mut self) -> &mut T {
        self.inner.as_mut().map(|x| x.as_mut()).unwrap()
    }
}

impl<T> Deref for UniquePtr<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        self.inner.as_ref().map(|x| x.as_ref()).unwrap()
    }
}

impl<T> DerefMut for UniquePtr<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.inner.as_mut().map(|x| x.as_mut()).unwrap()
    }
}

impl<T> PartialEq for UniquePtr<T> {
    fn eq(&self, other: &Self) -> bool {
        self.inner == other.inner
    }
}

impl<T> Eq for UniquePtr<T> {}

impl<T> Hash for UniquePtr<T> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.inner.hash(state)
    }
}

impl<T> Borrow<T> for UniquePtr<T> {
    fn borrow(&self) -> &T {
        self.inner.borrow()
    }
}

impl<T> BorrowMut<T> for UniquePtr<T> {
    fn borrow_mut(&mut self) -> &mut T {
        self.inner.borrow_mut()
    }
}

impl<T> ToOwned for UniquePtr<T> {
    type Owned = Self;

    fn to_owned(&self) -> Self::Owned {
        Self {
            inner: self.inner.to_owned(),
            is_array: self.is_array,
        }
    }
}

impl<T> From<UniquePtr<T>> for Box<dyn DerefMut<Target = T>> {
    fn from(unique_ptr: UniquePtr<T>) -> Self {
        unique_ptr.inner.unwrap()
    }
}

impl<T> From<UniquePtr<T>> for Vec<T> {
    fn from(unique_ptr: UniquePtr<T>) -> Self {
        unique_ptr.inner.unwrap().into_iter().collect()
    }
}
