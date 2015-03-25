/************************************************************************
 * Rados OSS Plugin for XRootD                                          *
 * Copyright © 2013 CERN/Switzerland                                    *
 *                                                                      *
 * Author: Joaquim Rocha <joaquim.rocha@cern.ch>                        *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <private/XrdOss/XrdOssError.hh>
#include <XrdOuc/XrdOucEnv.hh>
#include <XrdSfs/XrdSfsAio.hh>

#include <stdio.h>
#include <string>
#include <radosfs/File.hh>

#include "RadosOssFile.hh"
#include "RadosOssDefines.hh"

RadosOssFile::RadosOssFile(radosfs::Filesystem *radosFs,
                           const XrdSysError &eroute)
  : mRadosFs(radosFs),
    mFile(0),
    mObjectName(0),
    mEroute(eroute)
{
  fd = -1;
}

RadosOssFile::~RadosOssFile()
{
  delete mFile;
  free(mObjectName);
  mObjectName = 0;
}

int
RadosOssFile::Close(long long *retsz)
{
  Fsync();

  return XrdOssOK;
}

int
RadosOssFile::Open(const char *path, int flags, mode_t mode, XrdOucEnv &env)
{
  int ret = 0;
  mObjectName = strdup(path);
  radosfs::File::OpenMode openMode = radosfs::File::MODE_READ;

  if (flags & O_RDWR)
    openMode = (radosfs::File::OpenMode)
        (radosfs::File::MODE_WRITE | radosfs::File::MODE_READ);
  else if (flags & O_WRONLY)
    openMode = (radosfs::File::OpenMode) radosfs::File::MODE_WRITE;

  mFile = new radosfs::File(mRadosFs, path, openMode);

  if (flags & O_CREAT)
    ret = mFile->create(-1, std::string(""), env.Get("rfs.stripe") ? atoi(env.Get("rfs.stripe")) : 0 );

  if (flags & O_TRUNC)
    ret = mFile->truncate(0);

  return ret;
}

void
RadosOssFile::ReadCB(radosfs::Callback::callback_data* data)
{
  XrdSfsAio* aiop = (XrdSfsAio*) data->caller;
  aiop->Result = (!data->retc)?aiop->sfsAio.aio_nbytes:data->retc;
  aiop->doneRead();
}

void
RadosOssFile::WriteCB(radosfs::Callback::callback_data* data)
{
  XrdSfsAio* aiop = (XrdSfsAio*) data->caller;
  aiop->Result = (!data->retc)?aiop->sfsAio.aio_nbytes:-data->retc;
  aiop->doneWrite();
}

ssize_t
RadosOssFile::Read(off_t offset, size_t blen)
{
  if (fd < 0)
    return (ssize_t)-XRDOSS_E8004;

  return 0;
}

ssize_t
RadosOssFile::Read(void *buff, off_t offset, size_t blen)
{
  return mFile->read((char *) buff, offset, blen);
}

int 
RadosOssFile::Read(XrdSfsAio *aiop) 
{
  // we do synchronous read ... the asynchronous upstream part is unclear and creates SEGVs
  int rc = Read((char*)aiop->sfsAio.aio_buf, aiop->sfsAio.aio_offset, aiop->sfsAio.aio_nbytes);
  aiop->Result = rc;
  aiop->doneRead();
  return 0;

  /*
  std::vector<radosfs::FileReadData> readvector;
  readvector.push_back(radosfs::FileReadData((char*)aiop->sfsAio.aio_buf, aiop->sfsAio.aio_offset, aiop->sfsAio.aio_nbytes));
  radosfs::Callback callme(RadosOssFile::ReadCB, (void*) aiop);
  int rc =  mFile->read(readvector,0, callme);
  return rc;
  */
}


ssize_t
RadosOssFile::ReadRaw(void *buff, off_t offset, size_t blen) 
{
  return Read(buff, offset, blen);
}

ssize_t 
RadosOssFile::ReadV(XrdOucIOVec *readV, int n)
{
  std::vector<radosfs::FileReadData> rvec;
  std::vector<ssize_t> frd_retc;
  std::string asyncOpId;

  // pupulate the vector read data structure                                                                                                                                                  
  for (int i=0; i<n; ++i) {
    frd_retc.push_back(0);
    radosfs::FileReadData frd(readV[i].data, readV[i].offset, readV[i].size &frd_retc.back());
    rvec.push_back(frd);
  }

  ssize_t rbytes = mFile->read(rvec,&asyncOpId);

  mFile->sync(asyncOpId);
  
  // verify no vector read error                                                                                                                                                              
  for (size_t i = 0; i < frd_retc.size(); ++i) {
    if (frd_retc[i] < 0) {
      return -1;
    }
  }
}

int
RadosOssFile::Ftruncate(unsigned long long size)
{
  return mFile->truncate(size);
}

int
RadosOssFile::Fchmod(mode_t mode)
{
  return mFile->chmod(mode);
}

int
RadosOssFile::Fsync()
{
  return mFile->sync();
}

int
RadosOssFile::Fstat(struct stat *buff)
{
  return mRadosFs->stat(mObjectName, buff);
}

ssize_t
RadosOssFile::Write(const void *buff, off_t offset, size_t blen)
{
  int ret = 0;

  if (RadosOss::gWriteIsSync) {
    ret = mFile->writeSync((char *) buff, offset, blen);
  } else {
    ret = mFile->write((char *) buff, offset, blen);
  }
  // The libradosfs file write returns 0 if it succeeds but the XRootD OSS Write
  // needs to return the number of bytes instead
  if (ret == 0)
    ret = blen;

  return ret;
}

int 
RadosOssFile::Write(XrdSfsAio *aiop) 
{
  radosfs::Callback callme(RadosOssFile::WriteCB, (void*) aiop);
  
  return mFile->write((const char*)aiop->sfsAio.aio_buf, aiop->sfsAio.aio_offset, aiop->sfsAio.aio_nbytes, false, callme);
}

