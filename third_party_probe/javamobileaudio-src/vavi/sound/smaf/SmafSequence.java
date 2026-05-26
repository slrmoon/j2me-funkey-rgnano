/*
 * Copyright (c) 2007 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.sound.smaf;

import java.util.List;

import vavi.sound.smaf.chunk.Chunk;
import vavi.sound.smaf.chunk.FileChunk;
import vavi.sound.smaf.chunk.TrackChunk;
import vavi.sound.smaf.message.STMessage;
import vavi.sound.smaf.message.VNMessage;
import vavi.util.Debug;


/**
 * SmafSequence. 
 *
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 071012 nsano initial version <br>
 */
class SmafSequence extends Sequence {

    /** TODO ���e�� SmafFileFormat/FileChunk �Ɉڂ��ׂ� */
    SmafSequence(FileChunk fileChunk) {
        try {
            for (TrackChunk scoreTrackChunk : fileChunk.getScoreTrackChunks()) {
                createTrack(scoreTrackChunk.getSmafEvents());
            }
            for (TrackChunk pcmAudioTrackChunk : fileChunk.getPcmAudioTrackChunks()) {
                createTrack(pcmAudioTrackChunk.getSmafEvents());
            }
            for (TrackChunk graphicsTrackChunk : fileChunk.getGraphicsTrackChunks()) {
                createTrack(graphicsTrackChunk.getSmafEvents());
            }
            if (fileChunk.getMasterTrackChunk() != null) {
                createTrack(fileChunk.getMasterTrackChunk().getSmafEvents());
            }
            // SMF XF Information | SMAF Contents Info Chunk
            // -------------------+-------------------------
            // �Ȗ�               | ST: �Ȗ�
            // ��Ȏ�             | SW: ���
            // �쎌��             | WW: �쎌
            // �ҋȎ�             | AW: �ҋ�
            // ���t�ҁE�̏���     | AN: �A�[�e�B�X�g��
            Track track0 = tracks.get(0);
            String title = "title";
            String prot = "vavi";
            if (fileChunk.getOptionalDataChunk() != null) {
                for (Chunk dataChunk : fileChunk.getOptionalDataChunk().getDataChunks()) {
Debug.println(dataChunk);
                    // TODO
                }
            } else if (fileChunk.getContentsInfoChunk() != null) { // TODO �K�{�Ȃ̂� if �v���
                title = fileChunk.getContentsInfoChunk().getSubDataByTag("ST");
                prot = fileChunk.getContentsInfoChunk().getSubDataByTag("SW");
                // TODO create meta for ContentsInfoChunk
            }
            insert(track0, new SmafEvent(new VNMessage(prot == null ? "" : prot), 0), 0);
            insert(track0, new SmafEvent(new STMessage(title == null ? "" : title), 0), 0);
        } catch (InvalidSmafDataException e) {
            throw (RuntimeException) new IllegalStateException().initCause(e);
        }
    }

    /** */
    private void createTrack(List<SmafEvent> events) {
        Track track = createTrack();
        if (events != null) {
            for (SmafEvent event : events) {
                track.add(event);
            }
        }
    }

    /**
     * hack of Track
     */
    private void insert(Track track, SmafEvent event, int index) {
        try {
            Class<? extends Track> clazz = track.getClass();
//for (Field field : clazz.getDeclaredFields()) {
// System.err.println("field: " + field);
//}
            @SuppressWarnings("unchecked")
            List<SmafEvent> events = (List<SmafEvent>) clazz.getDeclaredField("events").get(track);
            events.add(index, event);
        } catch (Exception e) {
            throw (RuntimeException) new IllegalStateException().initCause(e);
        }
    }
}

/* */
