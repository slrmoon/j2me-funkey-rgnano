/*
 * Copyright (c) 2007 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf;

import java.io.Serializable;

import javax.swing.event.EventListenerList;


/**
 * MetaEvent �@�\�̃��[�e�B���e�B�N���X�ł��D
 * <li>javax.sound.midi �p�b�P�[�W�ɂ͂Ȃ��B(SAMF �I���W�i��)
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 071010 nsano initial version <br>
 */
class MetaSupport implements Serializable {

    /** The metaEvent listeners */
    private EventListenerList listenerList = new EventListenerList();

    /** {@link MetaEventListener} ��ǉ����܂��D */
    public void addMetaEventListener(MetaEventListener l) {
        listenerList.add(MetaEventListener.class, l);
    }

    /** {@link MetaEventListener} ���폜���܂��D */
    public void removeMetaEventListener(MetaEventListener l) {
        listenerList.remove(MetaEventListener.class, l);
    }

    /** meta message �𔭍s���܂��D */
    public void fireMeta(MetaMessage meta) {
        Object[] listeners = listenerList.getListenerList();
        for (int i = listeners.length - 2; i >= 0; i -= 2) {
            if (listeners[i] == MetaEventListener.class) {
                ((MetaEventListener) listeners[i + 1]).meta(meta);
            }
        }
    }
}

/* */
